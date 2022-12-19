package messages

import pb "github.com/sazikov-ad/networks/proto"

func (info KnownMessageInfo) WasMessageRead(msg *pb.Message) bool {
	if res, ok := info[msg.MessageUid]; ok {
		return res
	}
	return false
}
