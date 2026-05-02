# queue — FIFO, LIFO, and Priority Queues

Queue data structures, equivalent to Python's `queue` module.

Queues are represented as arrays. Use `queue.new()` to create one.

## Functions

### Creating Queues

#### queue.new()
Create a new empty FIFO queue.

```
let q = queue.new()
```

#### queue.from_array(arr)
Create a queue from an existing array.

```
let q = queue.from_array([1, 2, 3])
```

### FIFO Operations

#### queue.push(q, value)
Add value to the back of the queue.

```
queue.push(q, "hello")
```

#### queue.pop(q)
Remove and return the front value. Returns null if empty.

```
let item = queue.pop(q)
```

#### queue.peek(q)
Return the front value without removing it.

```
let front = queue.peek(q)
```

### Deque Operations

#### queue.push_left(q, value)
Add value to the front of the queue.

```
queue.push_left(q, "urgent")
```

#### queue.pop_right(q)
Remove and return the back value.

```
let back = queue.pop_right(q)
```

### Stack (LIFO) Operations

#### queue.push_stack(q, value)
Push value onto stack (same as push — adds to back).

```
let s = queue.new()
queue.push_stack(s, 10)
queue.push_stack(s, 20)
```

#### queue.pop_stack(q)
Pop from stack (removes from back — LIFO order).

```
queue.pop_stack(s)  // returns 20
```

### Priority Queue

#### queue.push_priority(q, value, priority)
Insert value with given priority. Lower number = higher priority.

```
let pq = queue.new()
queue.push_priority(pq, "low task", 3)
queue.push_priority(pq, "urgent task", 1)
queue.push_priority(pq, "normal task", 2)
queue.pop_priority(pq)  // returns "urgent task"
```

#### queue.pop_priority(q)
Remove and return the highest-priority (lowest number) item.

### Utilities

#### queue.size(q)
Return the number of items in the queue.

#### queue.is_empty(q)
Return `true` if queue is empty.

#### queue.clear(q)
Remove all items from the queue.

#### queue.to_array(q)
Return a new array with all queue items in order.

## Comparison to Python

| Python | RADS |
|--------|------|
| `queue.Queue()` | `queue.new()` |
| `q.put(item)` | `queue.push(q, item)` |
| `q.get()` | `queue.pop(q)` |
| `q.qsize()` | `queue.size(q)` |
| `q.empty()` | `queue.is_empty(q)` |
| `queue.LifoQueue()` | Use `queue.push_stack`/`queue.pop_stack` |
| `queue.PriorityQueue()` | Use `queue.push_priority`/`queue.pop_priority` |
