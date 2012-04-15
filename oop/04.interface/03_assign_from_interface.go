package main

import "fmt"

// ----------------------------------------------------------

type File struct {
}

func (f *File) Read(buf []byte) (n int, err error) {
	fmt.Println("File.Read")
	return
}

func (f *File) Write(buf []byte) (n int, err error) {
	fmt.Println("File.Write")
	return
}

// ----------------------------------------------------------

type Writer interface {
    Write(buf []byte) (n int, err error)
}

type ReadWriter interface {
    Read(buf []byte) (n int, err error)
    Write(buf []byte) (n int, err error)
}

type IStream interface {
    Write(buf []byte) (n int, err error)
    Read(buf []byte) (n int, err error)
}

// ----------------------------------------------------------

func main() {

	var file1 IStream = new(File)
	var file2 ReadWriter = file1
	var file3 IStream = file2
	var file4 Writer = file3

	file1.Read(nil)
	file2.Read(nil)
	file3.Read(nil)
	file4.Write(nil)
}

// ----------------------------------------------------------

