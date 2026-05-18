// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getUserFromRequest } from "@/lib/apiHelpers";

export async function GET(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  try {
    const user = await getUserFromRequest(request);
    const session = await prisma.session.findFirst({
      where: { id: params.id, ownerId: user.userId },
      include: {
        actions: { orderBy: { createdAt: "asc" } },
        messages: { orderBy: { timestamp: "asc" } },
      },
    });
    if (!session) return NextResponse.json({ error: "Session not found" }, { status: 404 });
    return NextResponse.json({
      ...session,
      createdAt: session.createdAt.toISOString(), updatedAt: session.updatedAt.toISOString(),
      actions: session.actions.map((a: { createdAt: Date; [k: string]: unknown }) => ({ ...a, createdAt: a.createdAt.toISOString() })),
      messages: session.messages.map((m: { timestamp: Date; [k: string]: unknown }) => ({ ...m, timestamp: m.timestamp.toISOString() })),
    });
  } catch (error) {
    console.error(`GET /api/sessions/${params.id} error:`, error);
    return NextResponse.json({ error: "Failed to fetch session" }, { status: 500 });
  }
}

export async function PUT(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  try {
    const user = await getUserFromRequest(request);
    const existing = await prisma.session.findFirst({ where: { id: params.id, ownerId: user.userId } });
    if (!existing) return NextResponse.json({ error: "Session not found" }, { status: 404 });

    const { actions: _a, messages: _m, ...updateData } = await request.json();
    void _a; void _m;

    const updated = await prisma.session.update({
      where: { id: params.id },
      data: updateData,
      include: {
        actions: { orderBy: { createdAt: "asc" } },
        messages: { orderBy: { timestamp: "asc" } },
      },
    });
    return NextResponse.json({
      ...updated,
      createdAt: updated.createdAt.toISOString(), updatedAt: updated.updatedAt.toISOString(),
      actions: updated.actions.map((a: { createdAt: Date; [k: string]: unknown }) => ({ ...a, createdAt: a.createdAt.toISOString() })),
      messages: updated.messages.map((m: { timestamp: Date; [k: string]: unknown }) => ({ ...m, timestamp: m.timestamp.toISOString() })),
    });
  } catch (error) {
    console.error(`PUT /api/sessions/${params.id} error:`, error);
    return NextResponse.json({ error: "Failed to update session" }, { status: 500 });
  }
}

export async function DELETE(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  try {
    const user = await getUserFromRequest(request);
    const existing = await prisma.session.findFirst({ where: { id: params.id, ownerId: user.userId } });
    if (!existing) return NextResponse.json({ error: "Session not found" }, { status: 404 });
    await prisma.session.delete({ where: { id: params.id } });
    return NextResponse.json({ success: true });
  } catch (error) {
    console.error(`DELETE /api/sessions/${params.id} error:`, error);
    return NextResponse.json({ error: "Failed to delete session" }, { status: 500 });
  }
}