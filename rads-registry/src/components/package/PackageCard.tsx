"use client"

import Link from "next/link"
import { Card, CardHeader, CardTitle, CardDescription, CardContent, CardFooter } from "@/components/ui/card"
import { Badge } from "@/components/ui/badge"
import { Button } from "@/components/ui/button"
import { Package } from "@/types/package"
import { Download, Star, Clock } from "lucide-react"
import { formatDate, formatNumber } from "@/lib/utils"

interface PackageCardProps {
  package: Package
}

export function PackageCard({ package: pkg }: PackageCardProps) {
  return (
    <Card className="package-card hover:shadow-lg transition-all">
      <CardHeader>
        <div className="flex items-start justify-between">
          <div className="flex-1">
            <Link href={`/packages/${pkg.name}`}>
              <CardTitle className="hover:text-primary cursor-pointer">
                {pkg.displayName || pkg.name}
              </CardTitle>
            </Link>
            <CardDescription className="mt-1">
              {pkg.description}
            </CardDescription>
          </div>
          {pkg.category && (
            <Badge variant="secondary" className="shrink-0">
              {pkg.category}
            </Badge>
          )}
        </div>
      </CardHeader>

      <CardContent>
        <div className="flex flex-wrap gap-2 mt-4">
          {pkg.keywords.slice(0, 4).map((keyword) => (
            <Badge key={keyword} variant="outline">
              {keyword}
            </Badge>
          ))}
          {pkg.keywords.length > 4 && (
            <Badge variant="outline">+{pkg.keywords.length - 4}</Badge>
          )}
        </div>
      </CardContent>

      <CardFooter className="flex items-center justify-between mt-4">
        <div className="flex items-center gap-4 text-sm text-muted-foreground">
          <div className="flex items-center gap-1">
            <Download className="h-4 w-4" />
            <span>{formatNumber(pkg.stats.downloads)}</span>
          </div>
          <div className="flex items-center gap-1">
            <Star className="h-4 w-4" />
            <span>{formatNumber(pkg.stats.stars)}</span>
          </div>
          <div className="flex items-center gap-1">
            <Clock className="h-4 w-4" />
            <span>{formatDate(pkg.stats.lastUpdated)}</span>
          </div>
        </div>

        <Button asChild size="sm">
          <Link href={`/packages/${pkg.name}`}>View</Link>
        </Button>
      </CardFooter>
    </Card>
  )
}
