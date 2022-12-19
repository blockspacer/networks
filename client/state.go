package client

import (
	"github.com/sazikov-ad/networks/client/messages"
	pb "github.com/sazikov-ad/networks/proto"
)

type State struct {
	messages       []*pb.Message
	sentMessages   []*pb.Message
	messagesInfo   messages.KnownMessageInfo
	messagesToSend chan *pb.Message
}

func NewClientState(info messages.KnownMessageInfo, sent []*pb.Message) *State {
	return &State{messages: make([]*pb.Message, 0), sentMessages: sent, messagesInfo: info, messagesToSend: make(chan *pb.Message)}
}

func (state *State) AddSentMessage(message *pb.Message) {
	state.sentMessages = append(state.sentMessages, message)
}

func (state *State) QueueSendMessage(message *pb.Message) {
	state.messagesToSend <- message
}

func (state *State) GetMessageForSendSync() *pb.Message {
	return <-state.messagesToSend
}

func (state *State) GetMessagesInfo() messages.KnownMessageInfo {
	return state.messagesInfo
}

func (state *State) StoreMessages(messages []*pb.Message) {
	state.messages = messages
}

func (state *State) LoadMessages() []*pb.Message {
	return state.messages
}

func (state *State) LoadSentMessages() []*pb.Message {
	return state.sentMessages
}

func (state *State) SetRead(messageUid uint64) {
	state.messagesInfo[messageUid] = true
}

func (state *State) GetMessageStatus(message *pb.Message) bool {
	return state.messagesInfo.WasMessageRead(message)
}
