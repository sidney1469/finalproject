import api from "./api";

export const getHealth = async () => {
    const response = await api.get('/planes')
    console.log(response.data)
    return response.data
}