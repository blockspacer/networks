package client

import (
	"context"
	"time"

	pb "github.com/sazikov-ad/networks/proto"
)

func sendMessage(client pb.ChatRpcClient, request *pb.SendRequest) *pb.SendResponse {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()
	response, err := client.SendMessage(ctx, request)
	if err != nil {
		return nil
	}
	return response
}

func receiveMessage(client pb.ChatRpcClient, request *pb.ReceiveRequest) *pb.ReceiveResponse {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()
	response, err := client.ReceiveMessage(ctx, request)
	if err != nil {
		return nil
	}
	return response
}

func receiveSendMessages(client pb.ChatRpcClient, request *pb.FromRequest) *pb.FromResponse {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()
	response, err := client.SendedMessages(ctx, request)
	if err != nil {
		return nil
	}
	return response
}
