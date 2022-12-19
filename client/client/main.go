package main

import (
	"context"
	"fmt"
	"sync/atomic"
	"time"

	"github.com/sazikov-ad/networks/client"
	"github.com/sazikov-ad/networks/client/configuration"
	"github.com/sazikov-ad/networks/client/constants"
	"github.com/sazikov-ad/networks/client/messages"
	"github.com/sazikov-ad/networks/client/ui"
	"github.com/sazikov-ad/networks/client/utils"
	pb "github.com/sazikov-ad/networks/proto"
	"google.golang.org/grpc"
)

func main() {
	config := configuration.ReadClientConfiguration(constants.ClientConfig())
	createDirectoryIfNotExists(config.ConfigDirectory)

	ctx, _ := context.WithTimeout(context.Background(), 5*time.Second)
	conn, e := grpc.DialContext(ctx, fmt.Sprintf("%s:%d", config.ServerHost, config.ServerPort), grpc.WithInsecure(), grpc.WithBlock())

	if e != nil {
		panic(e)
	}
	defer conn.Close()
	cl := pb.NewChatRpcClient(conn)

	known := messages.ReadMessageInfo(utils.JoinPaths(config.ConfigDirectory, constants.MessageData))
	state := client.NewClientState(known, loadSentMessage(cl))

	receiveMessages(cl, state)

	var shouldStop int32 = 0

	go func() {
		for r := atomic.LoadInt32(&shouldStop); r != 1; {
			receiveMessages(cl, state)
			time.Sleep(time.Duration(config.ServerBackoff) * time.Second)
		}
	}()

	go func() {
		for r := atomic.LoadInt32(&shouldStop); r != 1; {
			sendMessage(cl, state)
			time.Sleep(time.Duration(config.ServerBackoff) * time.Second)
		}
	}()

	ui := ui.CreateUserInterface(state)
	ui.Run()
	atomic.StoreInt32(&shouldStop, 1)
	messages.WriteMessageInfo(known, utils.JoinPaths(config.ConfigDirectory, constants.MessageData))
}
