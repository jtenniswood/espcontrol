const test = require('node:test');
const assert = require('node:assert/strict');
const { loadTypescriptTest } = require('./helpers/load_typescript_test');
const { ResetSession } = loadTypescriptTest('src/webserver/api/reset_session.ts');
const response = (body, status = 200) => new Response(JSON.stringify(body), { status });
const capabilities = (epoch = 7, pending = false) => ({ modes: ['customization', 'factory'], epoch, pending });

test('all mutations carry the original epoch; stale pages never acquire a new one', async () => {
  let epoch = 7;
  let calls = [];
  const session = new ResetSession(async (url, init) => {
    if (url === '/api/v1/reset') return response(capabilities(epoch));
    calls.push(init.headers.get('X-EspControl-Epoch'));
    return response({}, epoch === 7 ? 200 : 428);
  });
  await session.fetch('/text/layout/set', { method: 'POST' });
  epoch = 8;
  await session.fetch('/api/v1/config', { method: 'PUT' });
  await assert.rejects(session.fetch('/text/layout/set', { method: 'POST' }), /reload/);
  assert.deepEqual(calls, ['7', '7']);
});

test('reset has explicit intent, authentication credentials and mode, then blocks queued writes', async () => {
  const calls = [];
  const session = new ResetSession(async (url, init) => {
    calls.push([url, init]);
    return init.method === 'POST' ? response({}, 202) : response(capabilities());
  });
  await session.reset('customization');
  const init = calls[1][1];
  assert.equal(init.credentials, 'include');
  assert.equal(init.headers['X-EspControl-Request'], 'reset');
  assert.equal(init.headers['X-EspControl-Epoch'], '7');
  assert.equal(init.headers['Content-Type'], 'application/json');
  assert.equal(JSON.parse(init.body).mode, 'customization');
  await assert.rejects(session.fetch('/update', { method: 'POST' }), /reload/);
  assert.equal(calls.length, 2);
});

test('lost reset response and rejected reset both leave queued writes blocked', async () => {
  for (const networkError of [false, true]) {
    const session = new ResetSession(async (url, init) => {
      if (init.method !== 'POST') return response(capabilities());
      if (networkError) throw new Error('disconnected');
      return response({ error: 'Firmware installation in progress' }, 409);
    });
    await assert.rejects(session.reset('factory'));
    await assert.rejects(session.fetch('/switch/schedule/turn_on', { method: 'POST' }), /reload/);
  }
});

test('older firmware supports existing writes without exposing reset', async () => {
  const session = new ResetSession(async (url, init) => {
    if (url === '/api/v1/reset') return response({}, 404);
    assert.equal(init.headers.has('X-EspControl-Epoch'), false);
    return response({});
  });
  assert.equal(await session.discover(), null);
  await session.fetch('/text/layout/set', { method: 'POST' });
  await assert.rejects(session.reset('factory'), /does not support/);
});

test('discovery auth failures fail closed but permit retry', async () => {
  let authorized = false;
  const session = new ResetSession(async () => authorized ? response(capabilities()) : response({}, 401));
  await assert.rejects(session.discover());
  authorized = true;
  assert.equal((await session.discover()).epoch, 7);
});

test('native generation conflict alone does not invalidate the editing session', async () => {
  let writes = 0;
  const session = new ResetSession(async url => url === '/api/v1/reset' ? response(capabilities()) : response({}, ++writes === 1 ? 409 : 200));
  await session.fetch('/api/v1/config', { method: 'PUT' });
  assert.equal((await session.fetch('/api/v1/config', { method: 'PUT' })).status, 200);
});

test('reload waits for completion of a newer reset epoch', async () => {
  let status = capabilities();
  const session = new ResetSession(async () => response(status));
  await session.discover();
  assert.equal(await session.restarted(), false);
  status = capabilities(8, true);
  assert.equal(await session.restarted(), false);
  status = capabilities(8);
  assert.equal(await session.restarted(), true);
});
