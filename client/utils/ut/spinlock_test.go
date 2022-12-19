package ut

import (
	"sync"
	"testing"

	. "github.com/onsi/gomega"
	"github.com/sazikov-ad/networks/client/utils"
)

func TestSpinLockAsLocker(t *testing.T) {
	var lock sync.Locker
	lock = utils.NewSpinLock()

	lock.Lock()
	lock.Unlock()
}

func TestSpinLock(t *testing.T) {
	g := NewGomegaWithT(t)
	var lock = utils.NewSpinLock()

	g.Expect(lock.IsLocked()).To(BeFalse(), "created lock should be unlocked")

	lock.Lock()
	g.Expect(lock.IsLocked()).To(BeTrue(), "lock should lock")
	lock.Unlock()

	g.Expect(lock.IsLocked()).To(BeFalse(), "unlocked lock should be unlocked")

	g.Expect(lock.TryLock()).To(BeTrue(), "try lock should lock successfully")
	g.Expect(lock.IsLocked()).To(BeTrue(), "lock should be locked after try lock")
	g.Expect(lock.TryLock()).To(BeFalse(), "try lock shouldn't lock locked lock")
	lock.Unlock()
	g.Expect(lock.IsLocked()).To(BeFalse(), "unlocked lock should be unlocked")
}
