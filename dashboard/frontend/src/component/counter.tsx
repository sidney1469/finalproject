import {useState} from 'react'

const Counter = () => {
    const [count, setCount] = useState(1)

    return (
        <>
            <p>Count is: {count}</p>
            <button onClick={() => setCount(prev => prev + 1)}>Add</button>
        </>
    )
}

export default Counter