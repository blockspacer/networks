package messages

type KnownMessageInfo map[uint64]bool

func ReadMessageInfo(file string) KnownMessageInfo {
	data := readMessageInfo(file)
	if data == nil {
		return make(map[uint64]bool)
	}
	return data
}

func WriteMessageInfo(info KnownMessageInfo, file string) {
	writeMessageInfo(info, file)
}
