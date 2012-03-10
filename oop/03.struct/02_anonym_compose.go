package main

import "fmt"

type Base struct {
}

func (base *Base) Foo() {
	fmt.Println("Base.Foo")
}

func (base *Base) Bar() {
	fmt.Println("Base.Bar")
}

type Foo struct {
    Base
}

func (foo *Foo) Bar() {
    foo.Base.Bar()
	fmt.Println("Foo.Bar")
}

func main() {
	foo := new(Foo)
	foo.Foo()
	foo.Bar()
}

