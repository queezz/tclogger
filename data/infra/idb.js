export async function openDB(){
  return await new Promise((resolve, reject)=>{
    const req = indexedDB.open('tclogger-db', 1);
    req.onupgradeneeded = ()=>{
      const db = req.result;
      if(!db.objectStoreNames.contains('files')) db.createObjectStore('files', { keyPath: 'name' });
      if(!db.objectStoreNames.contains('blobs')) db.createObjectStore('blobs', { keyPath: 'key' });
      if(!db.objectStoreNames.contains('series')) db.createObjectStore('series', { keyPath: 'key' });
    };
    req.onsuccess = ()=> resolve(req.result);
    req.onerror = ()=> reject(req.error);
  });
}
export function tx(db, store, mode='readonly'){
  return db.transaction(store, mode).objectStore(store);
}
