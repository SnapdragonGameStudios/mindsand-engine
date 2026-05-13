// Copyright Qualcomm Innovation Center, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

import { PrismaClient } from "@prisma/client";
import { PrismaBetterSqlite3 } from "@prisma/adapter-better-sqlite3";
import { readFileSync } from "fs";
import { fileURLToPath } from "url";
import path from "path";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const root = path.join(__dirname, "..");

// Load .env manually
const envPath = path.join(root, ".env");
try {
  const envContent = readFileSync(envPath, "utf-8");
  for (const line of envContent.split("\n")) {
    const match = line.match(/^([^#=]+)=(.*)$/);
    if (match) {
      const key = match[1].trim();
      const val = match[2].trim().replace(/^["']|["']$/g, "");
      if (!process.env[key]) process.env[key] = val;
    }
  }
} catch { /* .env not found */ }

const dbUrl = process.env.DATABASE_URL ?? "file:./prisma/data/dev.db";
const dbPath = dbUrl.replace(/^file:/, "");
const resolvedPath = path.isAbsolute(dbPath) ? dbPath : path.join(root, dbPath);

const adapter = new PrismaBetterSqlite3({ url: resolvedPath });
const p = new PrismaClient({ adapter });

async function main() {
  await p.message.deleteMany();
  await p.action.deleteMany();
  await p.session.deleteMany();
  await p.testResult?.deleteMany?.();
  await p.unitTest?.deleteMany?.();
  await p.commitItem?.deleteMany?.();
  await p.commitRequest?.deleteMany?.();
  await p.gameObject.deleteMany();
  await p.libraryAction.deleteMany();
  await p.globalConfig.deleteMany();
  await p.user.deleteMany();
  console.log("DB cleared successfully");
  await p.$disconnect();
}

main().catch((e) => {
  console.error(e);
  process.exit(1);
});