import api from "./api"

const getPlanes = async () => {
    const response = await api.get('/planes/list')
    console.log(response.data)
    return response.data
}

export const getCoords = async () => {
    const response = await api.get('/planes/coords')
    console.log(response.data)
    return response.data
}


export default getPlanes;