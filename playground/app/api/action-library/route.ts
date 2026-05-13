import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getUserFromRequest, listFilter, serializeLibraryAction } from "@/lib/apiHelpers";

export async function GET(request: NextRequest) {
  try {
    const user = await getUserFromRequest(request);
    const actions = await prisma.libraryAction.findMany({
      where: listFilter(user.userId),
      orderBy: { createdAt: "desc" },
    });
    return NextResponse.json(actions.map(serializeLibraryAction));
  } catch (error) {
    console.error("GET /api/action-library error:", error);
    return NextResponse.json({ error: "Failed to fetch actions" }, { status: 500 });
  }
}

export async function POST(request: NextRequest) {
  try {
    const user = await getUserFromRequest(request);
    const body = await request.json();
    const { name, description, parameters } = body;

    if (!name) return NextResponse.json({ error: "name is required" }, { status: 400 });

    const action = await prisma.libraryAction.create({
      data: {
        ownerId: user.userId,
        name,
        description: description ?? "",
        parameters: Array.isArray(parameters) ? JSON.stringify(parameters) : (parameters ?? "[]"),
      },
    });

    return NextResponse.json(serializeLibraryAction(action), { status: 201 });
  } catch (error) {
    console.error("POST /api/action-library error:", error);
    return NextResponse.json({ error: "Failed to create action" }, { status: 500 });
  }
}