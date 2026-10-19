#!/usr/bin/env node
// Deploie le depot sur GitHub, declenche le build cloud et recupere l'exe.
//   node tools/deploy.mjs --token=ghp_xxx [--repo=RetracOGFN] [--user=moi]

import { execFileSync } from 'node:child_process';
import { mkdirSync, writeFileSync, existsSync } from 'node:fs';

const args = Object.fromEntries(process.argv.slice(2).map(a => {
  const [k, ...v] = a.replace(/^--/, '').split('=');
  return [k, v.join('=')];
}));

const token = args.token;
if (!token) { console.error('--token=ghp_xxx requis'); process.exit(1); }

const repo = args.repo || 'RetracOGFN';
const api = 'https://api.github.com';

const headers = {
  Authorization: `Bearer ${token}`,
  Accept: 'application/vnd.github+json',
  'User-Agent': 'retrac-deploy'
};

const git = (...a) => execFileSync('git', a, { stdio: 'inherit' });

async function apiCall(path, options = {}) {
  const res = await fetch(`${api}${path}`, { ...options, headers: { ...headers, ...options.headers } });
  const text = await res.text();
  const body = text ? JSON.parse(text) : {};
  if (!res.ok) throw new Error(`${res.status} ${path} : ${body.message || text}`);
  return body;
}

const me = await apiCall('/user');
const user = args.user || me.login;
console.log(`[i] compte : ${user}`);

try {
  await apiCall(`/repos/${user}/${repo}`);
  console.log(`[i] depot ${user}/${repo} existe deja`);
} catch {
  await apiCall('/user/repos', {
    method: 'POST',
    body: JSON.stringify({ name: repo, private: true, auto_init: false })
  });
  console.log(`[ok] depot ${user}/${repo} cree`);
}

if (!existsSync('.git')) git('init');
git('add', '-A');
try { git('commit', '-m', 'Retrac OGFN'); } catch {}
git('branch', '-M', 'main');
git('remote', 'remove', 'origin');
git('remote', 'add', 'origin', `https://${token}@github.com/${user}/${repo}.git`);
git('push', '-u', 'origin', 'main', '--force');

console.log('[i] build cloud declenche, attente...');
await apiCall(`/repos/${user}/${repo}/actions/workflows/build.yml/dispatches`, {
  method: 'POST',
  body: JSON.stringify({ ref: 'main' })
});

let run = null;
for (let i = 0; i < 60 && !run; i++) {
  await new Promise(r => setTimeout(r, 5000));
  const runs = await apiCall(`/repos/${user}/${repo}/actions/runs?per_page=1`);
  if (runs.workflow_runs?.length) run = runs.workflow_runs[0];
}

if (!run) { console.error('aucun run trouve'); process.exit(1); }
console.log(`[i] run ${run.id}`);

for (let i = 0; i < 180; i++) {
  const current = await apiCall(`/repos/${user}/${repo}/actions/runs/${run.id}`);
  if (current.status === 'completed') {
    if (current.conclusion !== 'success') { console.error(`[!] build ${current.conclusion}`); process.exit(1); }
    break;
  }
  await new Promise(r => setTimeout(r, 5000));
}

const artifacts = await apiCall(`/repos/${user}/${repo}/actions/runs/${run.id}/artifacts`);
const artifact = artifacts.artifacts?.find(a => a.name === 'RetracOGFN');
if (!artifact) { console.error('artefact absent'); process.exit(1); }

const zip = await fetch(`${api}/repos/${user}/${repo}/actions/artifacts/${artifact.id}/zip`, { headers });
mkdirSync('dist', { recursive: true });
writeFileSync('dist/RetracOGFN.zip', Buffer.from(await zip.arrayBuffer()));

console.log('[ok] dist/RetracOGFN.zip recupere (dezippe pour RetracOGFN.exe)');
