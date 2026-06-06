const request = require('supertest');
const path = require('path');

// Import the express app or server setup that registers the API routes
let app;
try {
  app = require(path.resolve('src/webserver/modules/api.js'));
} catch (e) {
  // If the module exports a router, we need to mount it on an express app
  const express = require('express');
  const testApp = express();
  const apiRouter = require(path.resolve('src/webserver/modules/api.js'));
  testApp.use(apiRouter);
  app = testApp;
}

describe("Protected endpoints reject unauthenticated requests", () => {
  const payloads = [
    { description: "no auth header at all", headers: {} },
    { description: "empty Authorization header", headers: { Authorization: "" } },
    { description: "malformed token", headers: { Authorization: "Bearer invalidtoken123" } },
    { description: "expired JWT-like token", headers: { Authorization: "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxIiwiZXhwIjoxfQ.fake" } },
  ];

  test.each(payloads)("rejects unauthenticated /update request: $description", async ({ headers }) => {
    const res = await request(app)
      .post("/update")
      .set(headers)
      .attach("firmware", Buffer.from("fake_firmware_binary_data"), "firmware.bin");

    expect([401, 403]).toContain(res.status);
  });
});