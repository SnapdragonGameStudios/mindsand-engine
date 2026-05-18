// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getUserFromRequest } from "@/lib/apiHelpers";

export async function PUT(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  try {
    await getUserFromRequest(request);
    const body = await request.json();
    const { actionName, parameters, description } = body;
    const updated = await prisma.action.update({
      where: { id: params.id },
      data: {
        ...(actionName !== undefined && { actionName }),
        ...(parameters !== undefined && { parameters }),
        ...(description !== undefined && { description }),
      },
    });
    return NextResponse.json({ ...updated, createdAt: updated.createdAt.toISOString() });
  } catch (error) {
    console.error(`PUT /api/actions/${params.id} error:`, error);
    return NextResponse.json({ error: "Failed to update action" }, { status: 500 });
  }
}

export async function DELETE(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  try {
    await getUserFromRequest(request);
    await prisma.action.delete({ where: { id: params.id } });
    return NextResponse.json({ success: true });
  } catch (error) {
    console.error(`DELETE /api/actions/${params.id} error:`, error);
    return NextResponse.json({ error: "Failed to delete action" }, { status: 500 });
  }
}