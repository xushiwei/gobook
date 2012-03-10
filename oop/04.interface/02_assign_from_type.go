package main

import "fmt"

// ----------------------------------------------------------

type Integer int

func (a Integer) Less(b Integer) bool {
	return a < b
}

func (a *Integer) Add(b Integer) {
	*a += b
}

// ----------------------------------------------------------

type Lesser interface {
	Less(b Integer) bool
}

type LessAdder interface {
	Less(b Integer) bool
	Add(b Integer)
}

// ----------------------------------------------------------

func main() {

	var a Integer = 1

	var b1 Lesser = a
	var b2 Lesser = &a
	if b1.Less(2) {
		fmt.Println(a, "Less 2")
	}
	if b2.Less(2) {
		fmt.Println(a, "Less 2")
	}

	var c LessAdder = &a
	if c.Less(2) {
		fmt.Println(a, "Less 2")
	}
	c.Add(2)
	fmt.Println("a =", a)
}

// ----------------------------------------------------------

