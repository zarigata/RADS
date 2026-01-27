"use client"

import { Button } from "@/components/ui/button"
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/card"
import { Package, Rocket, Terminal, Zap, Search } from "lucide-react"
import Link from "next/link"

export default function HomePage() {
  const features = [
    {
      icon: <Package className="h-8 w-8" />,
      title: "Discover Packages",
      description: "Browse and search through thousands of RADS packages for your next project.",
      link: "/packages",
    },
    {
      icon: <Terminal className="h-8 w-8" />,
      title: "Easy Installation",
      description: "Install packages with a single command using the rstar CLI.",
      link: "/docs#installation",
    },
    {
      icon: <Zap className="h-8 w-8" />,
      title: "Lightning Fast",
      description: "Packages compiled to native RADS for maximum performance.",
      link: "/docs#performance",
    },
    {
      icon: <Rocket className="h-8 w-8" />,
      title: "Bytecode Performance",
      description: "3-5x faster execution with stack-based bytecode VM and 50+ opcodes.",
      link: "https://github.com/zarigata/RADS/releases/tag/v0.0.6",
    },
    {
      icon: <BarChart className="h-8 w-8" />,
      title: "Profiler & Debugging",
      description: "Real-time profiling with flamegraphs and full debugger with breakpoints.",
      link: "https://github.com/zarigata/RADS/releases/tag/v0.0.6",
    },
    {
      icon: <Wifi className="h-8 w-8" />,
      title: "WebSocket & GraphQL",
      description: "Real-time web with 128+ concurrent connections and GraphQL server.",
      link: "https://github.com/zarigata/RADS/releases/tag/v0.0.6",
    },
    {
      icon: <Code2 className="h-8 w-8" />,
      title: "LSP Server",
      description: "Full Language Server Protocol support with Rust-based implementation for all editors.",
      link: "https://github.com/zarigata/RADS/releases/tag/v0.0.6",
    },
  ]

  const quickStart = [
    "npm install -g rads",
    "rstar install database-sqlite",
    "rads my-app.rads",
  ]

  return (
    <div className="container py-16">
      <section className="text-center space-y-8 py-16">
        <h1 className="text-5xl font-bold tracking-tight sm:text-6xl rads-text-gradient">
          RADS Package Registry
        </h1>
        <p className="text-xl text-muted-foreground max-w-2xl mx-auto">
          Discover, publish, and install RADS packages. The official package registry for the
          RADS programming language.
        </p>
        <div className="flex justify-center gap-4">
          <Button asChild size="lg">
            <Link href="/packages">
              <Search className="mr-2 h-5 w-5" />
              Browse Packages
            </Link>
          </Button>
          <Button asChild variant="outline" size="lg">
            <Link href="/publish">Publish Package</Link>
          </Button>
        </div>
      </section>

      <section className="grid grid-cols-1 md:grid-cols-3 gap-6 py-16">
        {features.map((feature) => (
          <Card key={feature.title} className="hover:shadow-lg transition-all">
            <CardHeader>
              <div className="text-primary mb-4">{feature.icon}</div>
              <CardTitle>{feature.title}</CardTitle>
              <CardDescription className="mt-2">
                {feature.description}
              </CardDescription>
            </CardHeader>
            <CardContent>
              <Button asChild variant="ghost" className="w-full">
                <Link href={feature.link}>Learn More</Link>
              </Button>
            </CardContent>
          </Card>
        ))}
      </section>

      <section className="py-16">
        <h2 className="text-3xl font-bold text-center mb-8">
          Quick Start
        </h2>
        <Card>
          <CardHeader>
            <CardTitle>Get Started in 3 Steps</CardTitle>
            <CardDescription>
              Install RADS, install packages, and start building.
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            {quickStart.map((command, index) => (
              <div key={index} className="flex items-center gap-4">
                <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-full bg-primary text-primary-foreground font-bold">
                  {index + 1}
                </div>
                <code className="flex-1 rounded-md bg-muted px-4 py-2 text-sm font-mono">
                  {command}
                </code>
                <Button
                  asChild
                  variant="ghost"
                  size="sm"
                  className="shrink-0"
                >
                  <Link href="/docs">Docs</Link>
                </Button>
              </div>
            ))}
          </CardContent>
        </Card>
      </section>

      <section className="py-16 text-center">
        <Card>
          <CardHeader>
            <Rocket className="h-12 w-12 text-primary mx-auto mb-4" />
            <CardTitle>Ready to Publish?</CardTitle>
            <CardDescription className="mt-2 max-w-lg mx-auto">
              Share your RADS package with the community. Publishing is free and easy.
            </CardDescription>
          </CardHeader>
          <CardContent>
            <Button asChild size="lg">
              <Link href="/publish">
                <Rocket className="mr-2 h-5 w-5" />
                Publish Your Package
              </Link>
            </Button>
          </CardContent>
        </Card>
      </section>
    </div>
  )
}
