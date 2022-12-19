package constants

import (
	"os/user"

	"github.com/sazikov-ad/networks/client/utils"
)

const clientConfig = ".chat.yml"
const MessageData = "messages.data"

func userDirectory() string {
	currentUser, err := user.Current()
	if err != nil {
		panic(err)
	}
	return currentUser.HomeDir
}

func ClientConfig() string {
	return utils.JoinPaths(userDirectory(), clientConfig)
}
