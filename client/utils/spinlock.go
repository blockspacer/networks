package utils

import (
	"runtime"
	"sync/atomic"
)

func atomicTryLock(value *uint32) bool {
	return atomic.CompareAndSwapUint32(value, 0, 1)
}

func atomicTryAndTryLock(value *uint32) bool {
	return atomic.LoadUint32(value) == 0 && atomicTryLock(value)
}

func atomicUnlock(value *uint32) {
	atomic.StoreUint32(value, 0)
}

type SpinLock struct {
	state uint32
}

func NewSpinLock() *SpinLock {
	return &SpinLock{state: 0}
}

func (l *SpinLock) Lock() {
	if !atomicTryLock(&l.state) {
		for ok := true; ok; ok = !atomicTryAndTryLock(&l.state) {
			runtime.Gosched()
		}
	}
}

func (l *SpinLock) TryLock() bool {
	return atomicTryLock(&l.state)
}

func (l *SpinLock) IsLocked() bool {
	return atomic.LoadUint32(&l.state) == 1
}

func (l *SpinLock) Unlock() {
	atomicUnlock(&l.state)
}
