import "./styles.css";
import type { Dispatch, SetStateAction } from "react";

export type Flight = {
  flightName: string;
  long: number;
  lat: number;
};

type DisplayFlightProps = Flight & {
  setSelectedFlight: Dispatch<SetStateAction<number[] | null>>;
};

const DisplayFlight = ({
  flightName,
  long,
  lat,
  setSelectedFlight,
}: DisplayFlightProps) => {
  return (
    <div className="flightList">
      <button
        className="button"
        onClick={() => setSelectedFlight([lat, long])}
      >
        <div className="box">
          <p className="flightname">{flightName}</p>
          <p className="coords">{long.toFixed(3)}</p>
          <p className="coords">{lat.toFixed(3)}</p>
        </div>

        <div className="view-container">
          <p className="viewbutton">View On Map</p>
        </div>
      </button>
    </div>
  );
};

export default DisplayFlight;