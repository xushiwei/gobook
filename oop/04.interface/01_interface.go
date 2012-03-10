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

func (f *File) Seek(off int64, whence int) (pos int64, err error) {
	fmt.Println("File.Seek")
	return
}

func (f *File) Close() error {
	fmt.Println("File.Close")
	return nil
}

// ----------------------------------------------------------

type IFile interface {
    Read(buf []byte) (n int, err error)
    Write(buf []byte) (n int, err error)
    Seek(off int64, whence int) (pos int64, err error)
    Close() error
}

type IReader interface {
    Read(buf []byte) (n int, err error)
}

type IWriter interface {
    Write(buf []byte) (n int, err error)
}

type ICloser interface {
    Close() error
}

// ----------------------------------------------------------

func main() {

	var f *File = new(File)
	var file1 IFile = f
	var file2 IReader = f
	var file3 IWriter = f
	var file4 ICloser = f

	file1.Seek(0, 0)
	file2.Read(nil)
	file3.Write(nil)
	file4.Close()
}

// ----------------------------------------------------------

