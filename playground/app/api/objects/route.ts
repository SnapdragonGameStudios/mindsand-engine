import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getUserFromRequest, listFilter, serializeObject } from "@/lib/apiHelpers";

export async function GET(request: NextRequest) {
  try {
    const user = await getUserFromRequest(request);
    const objects = await prisma.gameObject.findMany({
      where: listFilter(user.userId),
      orderBy: { createdAt: "desc" },
    });
    return NextResponse.json(objects.map(serializeObject));
  } catch (error) {
    console.error("GET /api/objects error:", error);
    return NextResponse.json({ error: "Failed to fetch objects" }, { status: 500 });
  }
}

export async function POST(request: NextRequest) {
  try {
    const user = await getUserFromRequest(request);
    const body = await request.json();
    const { objectId, name, description, location } = body;

    if (!objectId || !name) {
      return NextResponse.json({ error: "objectId and name are required" }, { status: 400 });
    }

    const obj = await prisma.gameObject.create({
      data: {
        ownerId: user.userId,
        objectId,
        name,
        description: description ?? "",
        locationX: Number(location?.x ?? 0),
        locationY: Number(location?.y ?? 0),
        locationZ: Number(location?.z ?? 0),
      },
    });
    return NextResponse.json(serializeObject(obj), { status: 201 });
  } catch (error) {
    console.error("POST /api/objects error:", error);
    return NextResponse.json({ error: "Failed to create object" }, { status: 500 });
  }
}