package configuration

import (
	"io/ioutil"
	"os"

	"gopkg.in/yaml.v2"
)

type ClientConfiguration struct {
	ConfigDirectory string `yaml:"config_dir"`
	ServerBackoff   uint32 `yaml:"server_poll_backoff"`
	ServerHost      string `yaml:"server_host"`
	ServerPort      uint32 `yaml:"server_port"`
}

func ReadClientConfiguration(file string) *ClientConfiguration {
	if _, err := os.Stat(file); os.IsNotExist(err) {
		return defaultClientConfiguration()
	}
	return readClientConfigurationFile(file)
}

func WriteClientConfiguration(config *ClientConfiguration, file string) {
	data, err := yaml.Marshal(config)
	check(err)
	check(ioutil.WriteFile(file, data, os.ModePerm))
}
