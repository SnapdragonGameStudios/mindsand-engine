import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getUserFromRequest, serializeObject } from "@/lib/apiHelpers";

export async function PUT(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  try {
    const user = await getUserFromRequest(request);
    const existing = await prisma.gameObject.findFirst({
      where: { id: params.id, ownerId: user.userId },
    });
    if (!existing) return NextResponse.json({ error: "Object not found" }, { status: 404 });

    const body = await request.json();
    const { objectId, name, description, location } = body;

    const updated = await prisma.gameObject.update({
      where: { id: params.id },
      data: {
        ...(objectId !== undefined && { objectId }),
        ...(name !== undefined && { name }),
        ...(description !== undefined && { description }),
        ...(location !== undefined && {
          locationX: Number(location.x ?? 0),
          locationY: Number(location.y ?? 0),
          locationZ: Number(location.z ?? 0),
        }),
      },
    });
    return NextResponse.json(serializeObject(updated));
  } catch (error) {
    console.error("PUT /api/objects/[id] error:", error);
    return NextResponse.json({ error: "Failed to update object" }, { status: 500 });
  }
}

export async function DELETE(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  try {
    const user = await getUserFromRequest(request);
    const existing = await prisma.gameObject.findFirst({
      where: { id: params.id, ownerId: user.userId },
    });
    if (!existing) return NextResponse.json({ error: "Object not found" }, { status: 404 });
    await prisma.gameObject.delete({ where: { id: params.id } });
    return NextResponse.json({ success: true });
  } catch (error) {
    console.error("DELETE /api/objects/[id] error:", error);
    return NextResponse.json({ error: "Failed to delete object" }, { status: 500 });
  }
}