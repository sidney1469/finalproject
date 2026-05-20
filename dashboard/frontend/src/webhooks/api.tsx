import axios from 'axios'

const URL = "http://localhost:8000"

const api = axios.create({
    baseURL: URL,
    headers: {
        'Content-Type': 'application/json',
    },
    timeout: 10000,
});

export default api;

