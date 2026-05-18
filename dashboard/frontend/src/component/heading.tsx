import type { ReactElement } from "react"

const Heading = () => {
    return (
        <div className="headerDiv">
                <h1 className='mainHeader' style={{flex: 1, fontFamily: 'Roboto Mono, sans-serif', alignItems: "flex-start"}}>
                  Wheat Wheywot
                </h1>
                <div className="names-row">
                  <h2 className="heading-2">Sidney Neil</h2>
                  <h2 className="heading-2">Xander Akinson </h2>
                  <h2 className="heading-2">Fiachra Richards </h2>
                </div> 
        </div>
    )
}
export default Heading