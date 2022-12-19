package messages

import (
	"time"

	"github.com/sazikov-ad/networks/client/utils"
	pb "github.com/sazikov-ad/networks/proto"
)

func BuildMessage(to []string, sendTime time.Time, message string, reply *string) *pb.Message {
	if reply == nil {
		return &pb.Message{From: utils.CurrentUser(), To: to, Message: message, SendTs: uint64(sendTime.Unix())}
	}
	return &pb.Message{From: utils.CurrentUser(), To: to, Message: message, SendTs: uint64(sendTime.Unix()), Reply: []string{*reply}}
}
