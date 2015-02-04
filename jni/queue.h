/*
 * Copyright (c) 2015 Ivan Valiulin
 *
 * This file is part of viaVR.
 *
 * viaVR is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * viaVR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with viaVR. If not, see http://www.gnu.org/licenses
 */

#pragma once
#include <mutex>

template <class T>
class queue {
public:
	queue (int size, videoInfo* f);
	~queue ();
	void push (T*);
	void pop (T*);
	void flush ();
	int getSize ();
	bool isEmpty ();
	bool isAlmostEmpty ();
	bool isFull ();

private:
	std::mutex _lock;
	const int capacity;
	int size;
	int first;
	T** buffer;
};

template <class T>
queue<T>::queue (int size, videoInfo* f) : capacity (size) {
	buffer = new T*[capacity];
	for (int i = 0; i < capacity; i++) {
		buffer[i] = new T (f);
	}
	queue::first = 0;
	queue::size = 0;
}

template <class T>
queue<T>::~queue () {
	for (int i = 0; i < capacity; i++)
		delete buffer[i];
	delete[] buffer;
}

template <class T>
void queue<T>::push (T* f) {
	_lock.lock ();
	int t = first + size;
	if (t > capacity - 1)
		t -= capacity;
	f->swap (*buffer[t]);
	size++;
	_lock.unlock ();
}

template <class T>
void queue<T>::pop (T* f) {
	_lock.lock ();
	int t = first++;
	if (first > capacity - 1)
		first -= capacity;
	f->swap (*buffer[t]);
	size--;
	_lock.unlock ();
}

template <class T>
void queue<T>::flush () {
	_lock.lock ();
	first = 0;
	size = 0;
	_lock.unlock ();
}

template <class T>
int queue<T>::getSize () {
	return size;
}

template <class T>
bool queue<T>::isEmpty () {
	return size == 0;
}

template <class T>
bool queue<T>::isAlmostEmpty () {
	return size < 2;
}

template <class T>
bool queue<T>::isFull () {
	return size == capacity;
}
