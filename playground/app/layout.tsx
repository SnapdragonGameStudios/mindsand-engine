import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "MindSand Playground",
  description: "MindSand LLM interaction platform",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className="dark">
      <body className="bg-gray-900 text-gray-100 min-h-screen antialiased">
        {children}
      </body>
    </html>
  );
}
