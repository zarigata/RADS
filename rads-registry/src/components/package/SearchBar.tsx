"use client"

import { useState, useEffect } from "react"
import { Input } from "@/components/ui/input"
import { Button } from "@/components/ui/button"
import { Search } from "lucide-react"
import { debounce } from "@/lib/utils"
import { SearchFilters } from "@/types/package"

interface SearchBarProps {
  onSearch: (filters: SearchFilters) => void
  placeholder?: string
  disabled?: boolean
}

export function SearchBar({ onSearch, placeholder = "Search packages...", disabled = false }: SearchBarProps) {
  const [query, setQuery] = useState("")

  const debouncedSearch = debounce((value: string) => {
    onSearch({ query: value })
  }, 300)

  useEffect(() => {
    debouncedSearch(query)
  }, [query])

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault()
    onSearch({ query })
  }

  return (
    <form onSubmit={handleSubmit} className="relative">
      <div className="relative flex items-center">
        <Search className="absolute left-3 h-5 w-5 text-muted-foreground" />
        <Input
          type="search"
          placeholder={placeholder}
          value={query}
          onChange={(e) => setQuery(e.target.value)}
          disabled={disabled}
          className="pl-10 pr-4 search-input"
        />
        <Button
          type="submit"
          variant="default"
          size="sm"
          className="absolute right-1 top-1/2 -translate-y-1/2"
          disabled={disabled || !query.trim()}
        >
          <Search className="h-4 w-4" />
        </Button>
      </div>
    </form>
  )
}
