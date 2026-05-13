import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getUserFromRequest, serializeLibraryAction } from "@/lib/apiHelpers";

export async function PUT(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  try {
    const user = await getUserFromRequest(request);
    const existing = await prisma.libraryAction.findFirst({
      where: { id: params.id, ownerId: user.userId },
    });
    if (!existing) return NextResponse.json({ error: "Action not found" }, { status: 404 });

    const body = await request.json();
    const { name, description, parameters } = body;

    const updated = await prisma.libraryAction.update({
      where: { id: params.id },
      data: {
        ...(name !== undefined && { name }),
        ...(description !== undefined && { description }),
        ...(parameters !== undefined && {
          parameters: Array.isArray(parameters) ? JSON.stringify(parameters) : parameters,
        }),
      },
    });

    return NextResponse.json(serializeLibraryAction(updated));
  } catch (error) {
    console.error("PUT /api/action-library/[id] error:", error);
    return NextResponse.json({ error: "Failed to update action" }, { status: 500 });
  }
}

export async function DELETE(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  try {
    const user = await getUserFromRequest(request);
    const existing = await prisma.libraryAction.findFirst({
      where: { id: params.id, ownerId: user.userId },
    });
    if (!existing) return NextResponse.json({ error: "Action not found" }, { status: 404 });
    await prisma.libraryAction.delete({ where: { id: params.id } });
    return NextResponse.json({ success: true });
  } catch (error) {
    console.error("DELETE /api/action-library/[id] error:", error);
    return NextResponse.json({ error: "Failed to delete action" }, { status: 500 });
  }
}