namespace config
{

inline constexpr const char* serviceName = "Train Station";

using MatrixR4DTraits = MatrixR4DTraitsDft;
using TrainDockSensorTraits = TrainDockSensorTraitsDft;
using RFIDBroadcasterTraits = RFIDBroadcasterTraitsDft;

inline constexpr std::array<KnownTrain, 3> knownTrains
{
  KnownTrain {
    { 0x00, 0x00, 0x00, 0x00, 0, 0, 0, 0, 0, 0 },
    { 0x0f01f811, 0x80180700, 0x60000060 }
  }
};

}
