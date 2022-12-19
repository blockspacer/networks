package main

import (
	"os"

	"github.com/sazikov-ad/networks/client"
	"github.com/sazikov-ad/networks/client/utils"
	pb "github.com/sazikov-ad/networks/proto"
)

func receiveMessages(cl pb.ChatRpcClient, state *client.State) {
	m, r := client.ReceiveMessage(cl, utils.CurrentUser())
	if r == client.StatusOk {
		state.StoreMessages(m)
	}
}

func loadSentMessage(cl pb.ChatRpcClient) []*pb.Message {
	m, r := client.ReceiveSendMessages(cl)
	if r == client.StatusOk {
		return m
	}
	return []*pb.Message{}
}

func sendMessage(cl pb.ChatRpcClient, state *client.State) {
	m := state.GetMessageForSendSync()
	if client.SendMessage(cl, m) == client.StatusOk {
		state.AddSentMessage(m)
	}
}

func createDirectoryIfNotExists(dir string) {
	if _, e := os.Stat(dir); os.IsNotExist(e) {
		_ = os.MkdirAll(dir, os.ModePerm)
	}
}
