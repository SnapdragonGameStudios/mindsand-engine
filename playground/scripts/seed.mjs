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
} catch { /* .env not found, continue */ }

const dbUrl = process.env.DATABASE_URL ?? "file:./prisma/data/dev.db";
const dbPath = dbUrl.replace(/^file:/, "");
const resolvedPath = path.isAbsolute(dbPath) ? dbPath : path.join(root, dbPath);

const adapter = new PrismaBetterSqlite3({ url: resolvedPath });
const prisma = new PrismaClient({ adapter });

async function main() {
  // Load seed data
  const seedData = JSON.parse(readFileSync(path.join(root, "prisma", "seed.json"), "utf-8"));

  // Load system prompt
  let systemPrompt = "";
  try {
    systemPrompt = readFileSync(path.join(root, "prisma", "systemprompt.txt"), "utf-8").trim();
  } catch { /* file not found */ }

  // Seed actions (only if none exist)
  const existingActions = await prisma.libraryAction.count();
  if (existingActions === 0) {
    for (const action of seedData.actions) {
      await prisma.libraryAction.create({
        data: {
          name: action.name,
          description: action.description,
          parameters: JSON.stringify(action.parameters),
        },
      });
    }
    console.log(`✓ Seeded ${seedData.actions.length} actions`);
  } else {
    console.log(`  Skipped actions (${existingActions} already exist)`);
  }

  // Seed objects (only if none exist)
  const existingObjects = await prisma.gameObject.count();
  if (existingObjects === 0) {
    for (const obj of seedData.objects) {
      await prisma.gameObject.create({
        data: {
          objectId: obj.objectId,
          name: obj.name,
          description: obj.description,
        },
      });
    }
    console.log(`✓ Seeded ${seedData.objects.length} objects`);
  } else {
    console.log(`  Skipped objects (${existingObjects} already exist)`);
  }

  // Upsert system prompt (always update from file)
  if (systemPrompt) {
    await prisma.globalConfig.upsert({
      where: { id: "singleton" },
      update: { systemPrompt },
      create: { id: "singleton", systemPrompt },
    });
    console.log("✓ System prompt loaded from prisma/systemprompt.txt");
  } else {
    await prisma.globalConfig.upsert({
      where: { id: "singleton" },
      update: {},
      create: { id: "singleton", systemPrompt: "" },
    });
    console.log("  System prompt is empty (edit prisma/systemprompt.txt to set it)");
  }

  console.log("Seed complete.");
}

main()
  .catch((e) => { console.error(e); process.exit(1); })
  .finally(() => prisma.$disconnect());