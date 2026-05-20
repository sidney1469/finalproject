import DisplayFlight from "./flight";
import type { Flight } from "./flight";
import type { Dispatch, SetStateAction } from "react";

type ListInputs = {
  flights?: Flight[] | null;
  setSelectedFlight: Dispatch<SetStateAction<number[] | null>>;
};

const FlightsList = ({ flights, setSelectedFlight }: ListInputs) => {
  return (
    <div>
      {flights?.map((flight) => (
        <DisplayFlight
          key={flight.flightName}
          flightName={flight.flightName}
          long={flight.long}
          lat={flight.lat}
          setSelectedFlight={setSelectedFlight}
        />
      ))}
    </div>
  );
};

export default FlightsList;