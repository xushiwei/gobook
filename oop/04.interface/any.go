package main

import "fmt"

func main() {
	var v1 interface{} = 1
	var v2 interface{} = "abc"
	var v3 interface{} = &v2
	var v4 interface{} = struct{ X int }{1}
	var v5 interface{} = &struct{ X int }{1}
	fmt.Println(v1, v2, v3, v4, v5)
}
