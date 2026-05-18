// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

import { NextRequest } from "next/server";
import { prisma } from "@/lib/prisma";

export interface RequestUser {
  userId: string;
}

/**
 * Extracts the user ID from request headers set by middleware.
 * Auto-creates a User record in the DB if it doesn't exist yet.
 */
export async function getUserFromRequest(request: NextRequest): Promise<RequestUser> {
  const userId = request.headers.get("x-user-id") ?? crypto.randomUUID();

  // Ensure user exists atomically
  await prisma.user.upsert({
    where: { id: userId },
    update: {},
    create: { id: userId, username: userId },
  });

  return { userId };
}

/**
 * Returns the Prisma ownership filter for LIST queries.
 * Returns user's own items plus global seed items (ownerId = null).
 */
export function listFilter(userId: string) {
  return {
    OR: [
      { ownerId: userId },
      { ownerId: null as null },
    ],
  };
}

/**
 * Serialize a GameObject from Prisma (flat locationX/Y/Z) to API shape (location object).
 */
export function serializeObject(obj: {
  id: string;
  objectId: string;
  name: string;
  description: string;
  locationX: number;
  locationY: number;
  locationZ: number;
  createdAt: Date;
  [key: string]: unknown;
}) {
  return {
    id: obj.id,
    objectId: obj.objectId,
    name: obj.name,
    description: obj.description,
    location: { x: obj.locationX, y: obj.locationY, z: obj.locationZ },
    createdAt: obj.createdAt.toISOString(),
  };
}

/**
 * Serialize a LibraryAction from Prisma (parameters as JSON string) to API shape.
 */
export function serializeLibraryAction(action: {
  id: string;
  name: string;
  description: string;
  parameters: string;
  createdAt: Date;
  [key: string]: unknown;
}) {
  return {
    id: action.id,
    name: action.name,
    description: action.description,
    parameters: (() => {
      try { return JSON.parse(action.parameters); } catch { return []; }
    })(),
    createdAt: action.createdAt.toISOString(),
  };
}
