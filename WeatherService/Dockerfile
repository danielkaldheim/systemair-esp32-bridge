FROM node:12
WORKDIR /usr/src/app

COPY . .
RUN yarn install

EXPOSE 3000
CMD [ "node", "index.js" ]
