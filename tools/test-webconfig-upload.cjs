// Runs the actual versioned WebConfig uploader with deterministic fault cases.
// Optional live use: OPENREMOTE_URL and OPENREMOTE_TOKEN in the environment,
// then node tools/test-webconfig-upload.cjs <html> <target> <file>.
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const html = fs.readFileSync(process.argv[2], 'utf8');
for (const m of html.matchAll(/<script\b[^>]*>([\s\S]*?)<\/script>/gi))
  if (m[1].trim()) new vm.Script(m[1]);
const start = html.indexOf('  async function chunkedUploadOnce(');
assert(start >= 0);
const source = html.slice(start, html.indexOf('\n  /*', start));
function uploader(overrides) {
  const ctx = {Number, Error, Promise, encodeURIComponent, cancelledUploads: {},
    UPLOAD_CHUNK_BYTES: 8, window: {setTimeout: fn => fn()}, ...overrides};
  vm.createContext(ctx);
  vm.runInContext(source, ctx);
  return ctx.chunkedUploadOnce;
}
async function regression() {
  for (const mode of ['ok', 'short-write', 'lost-response', 'permanent', 'finish-failure', 'cancel']) {
    let bytes = 0, sends = 0, finishes = 0, cancels = 0, failed = false;
    const upload = uploader({
      cancelledUploads: mode === 'cancel' ? {irdb: true} : {},
      remoteFetch: async url => {
        if (url.includes('/cancel')) { cancels++; return {}; }
        if (url.includes('/finish')) {
          finishes++;
          if (mode === 'finish-failure') throw new Error('checksum mismatch');
          return {bytes};
        }
        return {bytes};
      },
      sendOneChunk: async (target, blob, offset) => {
        sends++;
        assert.equal(offset, bytes);
        if (mode === 'permanent' && offset >= 8) throw new Error('SD write failed');
        if (!failed && mode === 'short-write') { failed = true; throw new Error('SD write failed'); }
        bytes += blob.size;
        if (!failed && mode === 'lost-response') { failed = true; throw new Error('response lost'); }
        return {bytes};
      }
    });
    if (['permanent', 'finish-failure', 'cancel'].includes(mode)) {
      await assert.rejects(upload('irdb', new Blob(['x'.repeat(24)]), null, 0));
      assert.equal(cancels, 1);
      if (mode === 'permanent') { assert.equal(sends, 8); assert.equal(bytes, 8); }
      if (mode === 'finish-failure') { assert.equal(finishes, 1); assert.equal(sends, 3); }
      if (mode === 'cancel') assert.equal(sends, 0);
    } else {
      assert.equal((await upload('irdb', new Blob(['x'.repeat(24)]), null, 0)).bytes, 24);
      assert.equal(finishes, 1);
      assert.equal(cancels, 0);
    }
    console.log('PASS', mode);
  }
}
async function live() {
  const base = process.env.OPENREMOTE_URL, token = process.env.OPENREMOTE_TOKEN;
  assert(base && token, 'Set OPENREMOTE_URL and OPENREMOTE_TOKEN');
  const [target, path] = process.argv.slice(3);
  const buffer = fs.readFileSync(path), blob = new Blob([buffer]);
  const table = Array.from({length:256}, (_, i) => {
    let c=i; for(let j=0;j<8;j++) c=c&1 ? 0xedb88320^(c>>>1) : c>>>1; return c>>>0;
  });
  let crc=0xffffffff;
  for(const b of buffer) crc=table[(crc^b)&255]^(crc>>>8);
  crc=(crc^0xffffffff)>>>0;
  const request = async (url, options={}) => {
    const response = await fetch(base+url, {...options,
      headers:{'X-OpenRemote-Token':token}, signal:AbortSignal.timeout(300000)});
    const data = await response.json();
    if(!response.ok || data.ok===false) throw new Error(data.error || 'HTTP '+response.status);
    return data;
  };
  let injected=false;
  const upload=uploader({UPLOAD_CHUNK_BYTES:192*1024, window:{setTimeout}, remoteFetch:request,
    sendOneChunk:async (target, part, offset) => {
      const form=new FormData(); form.append('chunk',part,'chunk.bin');
      const result=await request('/api/upload/chunk?target='+encodeURIComponent(target)+'&offset='+offset,{method:'POST',body:form});
      if(process.env.OPENREMOTE_TEST_LOST_REPLY==='1' && !injected && offset>1048576){
        injected=true; console.log('TEST: discard one committed chunk response');
        throw new Error('Simulated lost response');
      }
      return result;
    }});
  const began=Date.now(); let logged=0;
  console.log('Starting',target,blob.size,'bytes, CRC',crc.toString(16));
  const result=await upload(target,blob,(n,total)=>{
    if(Date.now()-logged>10000 || n===total){
      logged=Date.now(); console.log('Progress',n,'/',total,Math.round(n/(Date.now()-began))+' KB/s');
    }
  },crc);
  console.log('FINISHED',JSON.stringify(result),'seconds',(Date.now()-began)/1000);
}
(async()=>{ await regression(); if(process.argv[3]) await live(); })().catch(e=>{console.error(e.message);process.exitCode=1;});
