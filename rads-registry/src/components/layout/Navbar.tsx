"use client"

import Link from "next/link"
import { Button } from "@/components/ui/button"
import { Package as Icon, Github, Moon, Sun } from "lucide-react"
import { useState } from "react"

export function Navbar() {
  const [darkMode, setDarkMode] = useState(false)

  const toggleDarkMode = () => {
    setDarkMode(!darkMode)
    document.documentElement.classList.toggle("dark")
  }

  return (
    <nav className="border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60">
      <div className="container flex h-16 items-center justify-between">
        <div className="flex items-center gap-6">
          <Link href="/" className="flex items-center gap-2">
            <Icon className="h-6 w-6 text-primary" />
            <span className="font-bold text-xl rads-text-gradient">
              RADS Registry
            </span>
          </Link>
          <div className="hidden md:flex gap-6">
            <Link
              href="/packages"
              className="text-sm font-medium hover:text-primary transition-colors"
            >
              Packages
            </Link>
            <Link
              href="/publish"
              className="text-sm font-medium hover:text-primary transition-colors"
            >
              Publish
            </Link>
            <Link
              href="/docs"
              className="text-sm font-medium hover:text-primary transition-colors"
            >
              Docs
            </Link>
          </div>
        </div>

        <div className="flex items-center gap-4">
          <Button
            variant="ghost"
            size="icon"
            onClick={toggleDarkMode}
            aria-label="Toggle dark mode"
          >
            {darkMode ? <Sun className="h-5 w-5" /> : <Moon className="h-5 w-5" />}
          </Button>
          <Button asChild variant="outline">
            <Link href="/login">
              <Github className="mr-2 h-4 w-4" />
              Login
            </Link>
          </Button>
        </div>
      </div>
    </nav>
  )
}
