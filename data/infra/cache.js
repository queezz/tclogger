import { openDB, tx } from './idb.js';
let dbPromise;
async function getDB(){ dbPromise ||= openDB(); return await dbPromise; }

export async function putFileMeta(meta){ const db = await getDB(); return await new Promise((res,rej)=>{ const s=tx(db,'files','readwrite'); const r=s.put({ ...meta, lastSeen: Date.now() }); r.onsuccess=()=>res(true); r.onerror=()=>rej(r.error); }); }
export async function getAllFiles(){ const db = await getDB(); return await new Promise((res,rej)=>{ const s=tx(db,'files'); const r=s.getAll(); r.onsuccess=()=>res(r.result||[]); r.onerror=()=>rej(r.error); }); }

export async function putBlob(name, blob){ const db = await getDB(); return await new Promise((res,rej)=>{ const s=tx(db,'blobs','readwrite'); const r=s.put({ key: `${name}#raw`, blob, ts: Date.now() }); r.onsuccess=()=>res(true); r.onerror=()=>rej(r.error); }); }
export async function getBlob(name){ const db = await getDB(); return await new Promise((res,rej)=>{ const s=tx(db,'blobs'); const r=s.get(`${name}#raw`); r.onsuccess=()=>res(r.result?.blob||null); r.onerror=()=>rej(r.error); }); }

export async function putSeries(name, series, meta){ const db = await getDB(); return await new Promise((res,rej)=>{ const s=tx(db,'series','readwrite'); const r=s.put({ key: `${name}#v1`, series, meta, lastViewed: Date.now() }); r.onsuccess=()=>res(true); r.onerror=()=>rej(r.error); }); }
export async function getSeries(name){ const db = await getDB(); return await new Promise((res,rej)=>{ const s=tx(db,'series'); const r=s.get(`${name}#v1`); r.onsuccess=()=>res(r.result||null); r.onerror=()=>rej(r.error); }); }
