import type { Metadata } from "next"
import { Inter } from "next/font/google"
import "./globals.css"
import { Navbar } from "@/components/layout/Navbar"
import { Footer } from "@/components/layout/Footer"

const inter = Inter({ subsets: ["latin"] })

export const metadata: Metadata = {
  title: "RADS Package Registry",
  description: "Discover, publish, and install RADS packages. The official package registry for the RADS programming language.",
  keywords: ["RADS", "package registry", "programming language", "plugins"],
  authors: [{ name: "RADS Team" }],
  openGraph: {
    title: "RADS Package Registry",
    description: "Official package registry for RADS programming language",
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "RADS Package Registry",
  },
}

export default function RootLayout({
  children,
}: {
  children: React.ReactNode
}) {
  return (
    <html lang="en" suppressHydrationWarning>
      <body className={inter.className}>
        <div className="min-h-screen flex flex-col">
          <Navbar />
          <main className="flex-1">{children}</main>
          <Footer />
        </div>
      </body>
    </html>
  )
}
