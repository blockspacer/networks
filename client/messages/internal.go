package messages

import (
	"io/ioutil"
	"os"

	"github.com/kelindar/binary"
)

func check(e error) {
	if e != nil {
		panic(e)
	}
}

func readMessageInfo(file string) KnownMessageInfo {
	data, err := ioutil.ReadFile(file)
	if err != nil {
		return nil
	}
	result := KnownMessageInfo{}
	err = binary.Unmarshal(data, &result)
	if err != nil {
		return nil
	}
	return result
}

func writeMessageInfo(info KnownMessageInfo, file string) {
	data, err := binary.Marshal(info)
	check(err)
	check(ioutil.WriteFile(file, data, os.ModePerm))
}
