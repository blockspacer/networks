package client

import (
	"github.com/sazikov-ad/networks/client/utils"
	pb "github.com/sazikov-ad/networks/proto"
)

type RpcStatus uint8

const (
	StatusOk    RpcStatus = iota
	StatusError RpcStatus = iota
)

func SendMessage(client pb.ChatRpcClient, message *pb.Message) RpcStatus {
	response := sendMessage(client, &pb.SendRequest{Message: message})
	if response == nil {
		return StatusError
	}
	if response.Status == pb.Status_kOk {
		return StatusOk
	}
	return StatusError
}

func ReceiveMessage(client pb.ChatRpcClient, user string) ([]*pb.Message, RpcStatus) {
	response := receiveMessage(client, &pb.ReceiveRequest{User: user})
	if response == nil {
		return nil, StatusError
	}
	if response.Status == pb.Status_kOk {
		return response.Messages, StatusOk
	}
	return nil, StatusError
}

func ReceiveSendMessages(client pb.ChatRpcClient) ([]*pb.Message, RpcStatus) {
	response := receiveSendMessages(client, &pb.FromRequest{User: utils.CurrentUser()})
	if response.Status == pb.Status_kOk {
		return response.Messages, StatusOk
	}
	return nil, StatusError
}
