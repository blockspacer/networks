package utils

import (
	"os"
	"strings"
)

func JoinPaths(s ...string) string {
	return strings.Join(s, string(os.PathSeparator))
}
