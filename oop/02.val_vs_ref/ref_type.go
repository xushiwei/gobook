package main

import "fmt"

type IntegerRef struct {
	v *int
}

func (a *IntegerRef) Add(b int) {
	*a.v += b
}

func main() {
	var a = 1
	var b = IntegerRef{&a}
	b.Add(2)
	fmt.Println("a =", a)
}

