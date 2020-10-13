const request = require('request-promise');
const express = require('express');
const NodeCache = require('node-cache');

const app = express();
const port = 3000;
const nodeCache = new NodeCache({
  stdTTL: 10 * 60, // 10 minutes
});

const getLocationForecast = (lat, lon) => {
  return new Promise(async (resolve, reject) => {
    try {
      const options = {
        url: `https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=${lat}&lon=${lon}`,
        method: 'GET',
        json: true,
        headers: {
          'user-agent':
            'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36',
        },
      };
      const result = await request(options);
      resolve(result);
    } catch (error) {
      reject(error);
    }
  });
};

const windChill = (windSpeed, temperature) => {
  // (Wind Chill, °F) = 35.74 + 0.6215 × T - 35.75 × V 0.16 + 0.4275 × T × V 0.16
  const windSpeedMph = 2.23694 * windSpeed; // Convert from mps to mph
  const temperatureF = 1.8 * temperature + 32; // Convert from celsius to fahrenheit
  const result =
    35.74 +
    0.6215 * temperatureF -
    35.75 * Math.pow(windSpeedMph, 0.16) +
    0.4275 * temperatureF * Math.pow(windSpeedMph, 0.16);
  return parseFloat((0.55556 * (result - 32)).toFixed(2)); // Convert from fahrenheit to celsius
};

// Todo: Handle data better, handle error better
app.get('/locationforecast', async (req, res) => {
  const query = req.query;
  const key = `locationforecast-${query.lat}-${query.lon}`;
  if (nodeCache.get(key)) {
    res.setHeader('n-cache-status', 'HIT');
    const data = JSON.parse(nodeCache.get(key));
    res.status(200).json(data);
  } else {
    try {
      const result = await getLocationForecast(query.lat, query.lon);
      const next = result.properties.timeseries.splice(1, 1).shift();

      const response = {
        time: next.time,
        ...next.data.instant.details,
        // wind_chill: windChill(
        //   next.data.instant.details.wind_speed,
        //   next.data.instant.details.air_temperature
        // ),
      };
      data = JSON.stringify(response);
      nodeCache.set(key, data);
      res.setHeader('n-cache-status', 'MISS');
      res.status(200).json(response);
    } catch (error) {
      res.status(error.statusCode).send(error.message);
    }
  }
});

app.get('/next24', async (req, res) => {
  const query = req.query;
  const key = `next24-${query.lat}-${query.lon}`;
  if (nodeCache.get(key)) {
    res.setHeader('n-cache-status', 'HIT');
    const data = JSON.parse(nodeCache.get(key));
    res.status(200).json(data);
  } else {
    try {
      const result = await getLocationForecast(query.lat, query.lon);
      const timeseries = result.properties.timeseries;
      const next24 = timeseries.splice(1, 24);

      let lowAirTemperature = undefined;
      let highAirTemperature = undefined;
      let meanAirTemperature = 0;
      let lowWindSpeed = undefined;
      let highWindSpeed = undefined;
      let meanWindSpeed = 0;
      let lowCloud = undefined;
      let highCloud = undefined;
      let meanCloud = 0;
      const temperatures = [];
      next24.forEach((data) => {
        temperatures.push(data.data.instant.details.air_temperature);
        meanAirTemperature += data.data.instant.details.air_temperature;
        meanWindSpeed += data.data.instant.details.wind_speed;
        meanCloud += data.data.instant.details.cloud_area_fraction;
        if (
          !lowAirTemperature ||
          lowAirTemperature > data.data.instant.details.air_temperature
        ) {
          lowAirTemperature = data.data.instant.details.air_temperature;
        }
        if (
          !highAirTemperature ||
          highAirTemperature < data.data.instant.details.air_temperature
        ) {
          highAirTemperature = data.data.instant.details.air_temperature;
        }
        if (
          !lowWindSpeed ||
          lowWindSpeed > data.data.instant.details.wind_speed
        ) {
          lowWindSpeed = data.data.instant.details.wind_speed;
        }
        if (
          !highWindSpeed ||
          highWindSpeed < data.data.instant.details.wind_speed
        ) {
          highWindSpeed = data.data.instant.details.wind_speed;
        }
        if (
          !lowCloud ||
          lowCloud > data.data.instant.details.cloud_area_fraction
        ) {
          lowCloud = data.data.instant.details.cloud_area_fraction;
        }
        if (
          !highCloud ||
          highCloud < data.data.instant.details.cloud_area_fraction
        ) {
          highCloud = data.data.instant.details.cloud_area_fraction;
        }
      });
      meanAirTemperature /= next24.length;
      meanWindSpeed /= next24.length;
      meanCloud /= next24.length;

      const response = {
        time: `${next24[0].time}-${next24[next24.length - 1].time}`,
        temp: {
          low: lowAirTemperature,
          high: highAirTemperature,
          mean: parseFloat(meanAirTemperature.toPrecision(2)),
          // points: temperatures,
        },
        wind: {
          low: lowWindSpeed,
          high: highWindSpeed,
          mean: parseFloat(meanWindSpeed.toPrecision(2)),
        },
        cloud: {
          low: lowCloud,
          high: highCloud,
          mean: parseFloat(meanCloud.toPrecision(2)),
        },
      };
      data = JSON.stringify(response);
      nodeCache.set(key, data);
      res.setHeader('n-cache-status', 'MISS');
      res.status(200).json(response);
    } catch (error) {
      res.status(error.statusCode).send(error.message);
    }
  }
});

app.listen(port, () => {
  console.log(`App listening at http://localhost:${port}`);
});
