"use client";

import Header from "@/components/Header";
import Sidebar from "@/components/Sidebar";
import ChatPanel from "@/components/ChatPanel";
import ConfigPanel from "@/components/ConfigPanel";

export default function SessionsPage() {
  return (
    <div className="h-screen flex flex-col overflow-hidden bg-gray-900">
      <Header />
      <div className="flex flex-1 overflow-hidden">
        <Sidebar />
        <ChatPanel />
        <ConfigPanel />
      </div>
    </div>
  );
}
