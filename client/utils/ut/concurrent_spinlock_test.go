package ut

import (
	"testing"

	. "github.com/onsi/gomega"
	"github.com/sazikov-ad/networks/client/utils"
)

func hammerSpinLock(lock *utils.SpinLock, loops int, cdone chan bool, counter *uint64) {
	for i := 0; i < loops; i++ {
		lock.Lock()
		*counter++
		lock.Unlock()
	}
	cdone <- true
}

func TestConcurrentSpinLock(t *testing.T) {
	g := NewGomegaWithT(t)

	counter := uint64(0)
	loops := 1000
	l := utils.NewSpinLock()
	c := make(chan bool)

	for i := 0; i < 10; i++ {
		go hammerSpinLock(l, loops, c, &counter)
	}

	for i := 0; i < 10; i++ {
		<-c
	}

	g.Expect(counter).To(Equal(uint64(10 * loops)))
}
