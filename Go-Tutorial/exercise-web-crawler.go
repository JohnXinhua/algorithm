package main

import (
	"fmt"
)

type Fetcher interface {
	// Fetch returns the body of URL and
	// a slice of URLs found on that page.
	Fetch(url string) (body string, urls []string, err error)
}

type result struct {
	url, body string
	urls      []string
	err       error
	depth     int
}

// Crawl uses fetcher to recursively crawl
// pages starting with url, to a maximum of depth.
func Crawl(url string, depth int, fetcher Fetcher) {
	// Fetch URLs in parallel.
	// Don't fetch the same URL twice.
	fetched := make(map[string]bool)
	ch := make(chan *result)
	fetch := func(url string, depth int) {
		body, urls, err := fetcher.Fetch(url)
		ch <- &result{url, body, urls, err, depth}
	}

	go fetch(url, depth)
	inque := 1
	fetched[url] = true

	for ; inque > 0; inque-- {
		res := <-ch
		if res.err != nil {
			fmt.Println(res.err)
			continue
		}
		fmt.Printf("found: %s %q\n", res.url, res.body)

		if depth <= 0 {
			continue
		}

		for _, u := range res.urls {
			if !fetched[u] {
				go fetch(u, depth-1)
				inque++
				fetched[u] = true
			}
		}
	}
}

func main() {
	Crawl("http://golang.org/", 4, fetcher)
}

// fakeFetcher is Fetcher that returns canned results.
type fakeFetcher map[string]*fakeResult

type fakeResult struct {
	body string
	urls []string
}

func (f fakeFetcher) Fetch(url string) (string, []string, error) {
	if res, ok := f[url]; ok {
		return res.body, res.urls, nil
	}
	return "", nil, fmt.Errorf("not found: %s", url)
}

// fetcher is a populated fakeFetcher.
var fetcher = fakeFetcher{
	"http://golang.org/": &fakeResult{
		"The Go Programming Language",
		[]string{
			"http://golang.org/pkg/",
			"http://golang.org/cmd/",
		},
	},
	"http://golang.org/pkg/": &fakeResult{
		"Packages",
		[]string{
			"http://golang.org/",
			"http://golang.org/cmd/",
			"http://golang.org/pkg/fmt/",
			"http://golang.org/pkg/os/",
		},
	},
	"http://golang.org/pkg/fmt/": &fakeResult{
		"Package fmt",
		[]string{
			"http://golang.org/",
			"http://golang.org/pkg/",
		},
	},
	"http://golang.org/pkg/os/": &fakeResult{
		"Package os",
		[]string{
			"http://golang.org/",
			"http://golang.org/pkg/",
		},
	},
}

