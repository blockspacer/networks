package configuration

import (
	"io/ioutil"
	"os/user"

	"github.com/sazikov-ad/networks/client/utils"
	"gopkg.in/yaml.v2"
)

func check(e error) {
	if e != nil {
		panic(e)
	}
}

func userDirectory() string {
	currentUser, err := user.Current()
	check(err)
	return currentUser.HomeDir
}

func createClientConfiguration(dir string, backoff uint32, port uint32) *ClientConfiguration {
	return &ClientConfiguration{ConfigDirectory: dir, ServerBackoff: backoff, ServerPort: port}
}

func defaultClientConfiguration() *ClientConfiguration {
	return createClientConfiguration(utils.JoinPaths(userDirectory(), ".chat"), 5, 1234)
}

func readClientConfigurationFile(file string) *ClientConfiguration {
	data, err := ioutil.ReadFile(file)
	check(err)
	config := &ClientConfiguration{}
	err = yaml.Unmarshal(data, config)
	check(err)
	return config
}
