const test = require('node:test');
const assert = require('node:assert/strict');
const { loadTypescriptTest } = require('./helpers/load_typescript_test');
const { ResetSession } = loadTypescriptTest('src/webserver/api/reset_session.ts');
const response = (body, status = 200) => new Response(JSON.stringify(body), { status });
const capabilities = (epoch = 7, pending = false) => ({ modes: ['customization', 'factory'], epoch, pending });

const advertisement = { api: { version: 1 }, reset: { modes: ['customization', 'factory'], status: '/api/v1/reset' } };
function supportedSession(transport) {
  return new ResetSession((url, init) => url === '/api/v1/capabilities' ? response(advertisement) : transport(url, init));
}

test('all mutations carry the original epoch; stale pages never acquire a new one', async () => {
  let epoch = 7;
  let calls = [];
  const session = supportedSession(async (url, init) => {
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
  const session = supportedSession(async (url, init) => {
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
    const session = supportedSession(async (url, init) => {
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
    if (url === '/api/v1/capabilities') return response({ api: { version: 1 } });
    assert.notEqual(url, '/api/v1/reset');
    assert.equal(init.headers.has('X-EspControl-Epoch'), false);
    return response({});
  });
  assert.equal(await session.discover(), null);
  await session.fetch('/text/layout/set', { method: 'POST' });
  await assert.rejects(session.reset('factory'), /does not support/);
});

test('discovery auth failures fail closed but permit retry', async () => {
  let authorized = false;
  const session = supportedSession(async () => authorized ? response(capabilities()) : response({}, 401));
  await assert.rejects(session.discover());
  authorized = true;
  assert.equal((await session.discover()).epoch, 7);
});

test('native generation conflict alone does not invalidate the editing session', async () => {
  let writes = 0;
  const session = supportedSession(async url => url === '/api/v1/reset' ? response(capabilities()) : response({}, ++writes === 1 ? 409 : 200));
  await session.fetch('/api/v1/config', { method: 'PUT' });
  assert.equal((await session.fetch('/api/v1/config', { method: 'PUT' })).status, 200);
});

test('reload waits for completion of a newer reset epoch', async () => {
  let status = capabilities();
  const session = supportedSession(async () => response(status));
  await session.discover();
  assert.equal(await session.restarted(), false);
  status = capabilities(8, true);
  assert.equal(await session.restarted(), false);
  status = capabilities(8);
  assert.equal(await session.restarted(), true);
});


test('legacy capability discovery is shared and never probes an absent reset endpoint', async () => {
  const calls = [];
  const session = new ResetSession(async (url, init) => {
    calls.push(url);
    if (url === '/api/v1/capabilities') {
      assert.equal(init.credentials, 'include');
      return response({ api: { version: 1 }, web_assets: { versions: [1] } });
    }
    if (url === '/api/v1/reset') throw new TypeError('Failed to fetch');
    assert.equal(init.headers.has('X-EspControl-Epoch'), false);
    return response({});
  });
  await Promise.all([session.discover(), session.fetch('/api/v1/config', { method: 'PUT' }), session.fetch('/text/layout/set', { method: 'POST' })]);
  await session.fetch('/text/layout/set', { method: 'POST' });
  assert.equal(calls.filter(url => url === '/api/v1/capabilities').length, 1);
  assert.equal(calls.includes('/api/v1/reset'), false);
});

test('startup 404 and 503 keep concurrent saves waiting until reset support is known', async t => {
  const timers = [], calls = [], epochs = [];
  t.mock.method(globalThis, 'setTimeout', (callback, delay) => { timers.push({ callback, delay }); });
  const startup = [404, 503];
  const session = new ResetSession(async (url, init) => {
    calls.push(url);
    if (url === '/api/v1/capabilities') {
      assert.equal(init.credentials, 'include');
      const status = startup.shift();
      return status ? response({}, status) : response(advertisement);
    }
    if (url === '/api/v1/reset') return response(capabilities());
    epochs.push(init.headers.get('X-EspControl-Epoch'));
    return response({});
  });
  const discovery = session.discover();
  const writes = Promise.all([
    session.fetch('/api/v1/config', { method: 'PUT' }),
    session.fetch('/text/layout/set', { method: 'POST' }),
  ]);
  for (let retry = 0; retry < 2; retry++) {
    await new Promise(resolve => setImmediate(resolve));
    assert.deepEqual(epochs, [], 'no mutation may run during startup discovery');
    assert.equal(calls.includes('/api/v1/reset'), false);
    assert.equal(timers.length, 1, 'concurrent callers share a single retry');
    const timer = timers.shift();
    assert.equal(timer.delay, 2000);
    timer.callback();
  }
  await writes;
  assert.deepEqual((await discovery).modes, ['customization', 'factory']);
  assert.deepEqual(epochs, ['7', '7']);
  assert.equal(calls.filter(url => url === '/api/v1/capabilities').length, 3);
  assert.equal(calls.filter(url => url === '/api/v1/reset').length, 1);
});

test('startup failure only becomes cached legacy mode after a valid legacy response', async t => {
  const delays = [];
  t.mock.method(globalThis, 'setTimeout', (callback, delay) => { delays.push(delay); queueMicrotask(callback); });
  let requests = 0;
  const session = new ResetSession(async (url, init) => {
    if (url === '/api/v1/capabilities') return ++requests === 1
      ? response({}, 404) : response({ api: { version: 1 } });
    assert.notEqual(url, '/api/v1/reset');
    assert.equal(init.headers.has('X-EspControl-Epoch'), false);
    return response({});
  });
  assert.equal(await session.discover(), null);
  await session.fetch('/text/layout/set', { method: 'POST' });
  assert.equal(await session.discover(), null);
  assert.equal(requests, 2);
  assert.deepEqual(delays, [2000]);
});

test('exhausted startup retries block writes but a later attempt recovers without reloading', async t => {
  t.mock.method(globalThis, 'setTimeout', callback => queueMicrotask(callback));
  for (const status of [404, 503]) {
    let ready = false, requests = 0;
    const epochs = [];
    const session = new ResetSession(async (url, init) => {
      if (url === '/api/v1/capabilities') { requests++; return response(advertisement, ready ? 200 : status); }
      if (url === '/api/v1/reset') return response(capabilities());
      epochs.push(init.headers.get('X-EspControl-Epoch'));
      return response({});
    });
    await assert.rejects(session.fetch('/api/v1/config', { method: 'PUT' }), /capabilities/);
    assert.equal(requests, 4, 'startup retries are bounded');
    assert.deepEqual(epochs, []);
    ready = true;
    await session.fetch('/api/v1/config', { method: 'PUT' });
    assert.equal(requests, 5);
    assert.deepEqual(epochs, ['7']);
    assert.equal((await session.discover()).epoch, 7);
  }
});

test('invalid or unavailable capabilities block writes and discovery can retry', async t => {
  t.mock.method(globalThis, 'setTimeout', callback => queueMicrotask(callback));
  const failures = [
    () => { throw new TypeError('Failed to fetch'); },
    ...[401, 403, 404, 500, 503].map(status => () => response({}, status)),
    () => new Response('not json'),
    ...[null, [], {}, { api: { version: 2 } },
      { api: { version: 1 }, reset: null },
      { ...advertisement, reset: { ...advertisement.reset, status: 'https://other.test/reset' } },
      { ...advertisement, reset: { ...advertisement.reset, modes: [] } },
      { ...advertisement, reset: { ...advertisement.reset, modes: ['unknown'] } },
    ].map(value => () => response(value)),
  ];
  for (const failure of failures) {
    let fail = true, writes = 0;
    const session = new ResetSession(async url => {
      if (url === '/api/v1/capabilities') return fail ? failure() : response(advertisement);
      if (url === '/api/v1/reset') return response(capabilities());
      writes++; return response({});
    });
    await assert.rejects(session.fetch('/api/v1/config', { method: 'PUT' }));
    assert.equal(writes, 0);
    fail = false;
    await session.fetch('/api/v1/config', { method: 'PUT' });
    assert.equal(writes, 1);
  }
});

test('advertised reset failures never downgrade to unprotected writes', async () => {
  for (const failure of [
    () => { throw new TypeError('Failed to fetch'); },
    ...[401, 404, 500, 503].map(status => () => response({}, status)),
    () => response({}), () => new Response('not json'),
  ]) {
    let writes = 0;
    const session = supportedSession(async url => {
      if (url === '/api/v1/reset') return failure();
      writes++; return response({});
    });
    await assert.rejects(session.fetch('/api/v1/config', { method: 'PUT' }));
    assert.equal(writes, 0);
  }
});


test('a pending reset discovered on page load blocks every mutation', async () => {
  let writes = 0;
  const session = supportedSession(async url => {
    if (url === '/api/v1/reset') return response(capabilities(7, true));
    writes++; return response({});
  });
  await assert.rejects(session.fetch('/api/v1/config', { method: 'PUT' }), /reload/);
  await assert.rejects(session.fetch('/text/layout/set', { method: 'POST' }), /reload/);
  assert.equal(writes, 0);
});
