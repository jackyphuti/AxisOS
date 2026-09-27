import React, { useState, useEffect } from 'react';
import {
  Sun,
  Cloud,
  CloudRain,
  CloudLightning,
  Wind,
  Droplets,
  Eye,
  Compass,
  Search,
  Sunrise,
  Sunset,
  Thermometer,
} from 'lucide-react';

interface CityWeather {
  city: string;
  country: string;
  tempC: number;
  condition: 'Sunny' | 'Partly Cloudy' | 'Cloudy' | 'Rain' | 'Thunderstorm';
  highC: number;
  lowC: number;
  windKmh: number;
  humidity: number;
  uvIndex: number;
  pressureHpa: number;
  visibilityKm: number;
  sunrise: string;
  sunset: string;
  hourly: { time: string; tempC: number; icon: string }[];
  daily: { day: string; condition: string; minC: number; maxC: number; icon: string }[];
}

const PRESET_CITIES: CityWeather[] = [
  {
    city: 'San Francisco',
    country: 'United States',
    tempC: 19,
    condition: 'Partly Cloudy',
    highC: 22,
    lowC: 13,
    windKmh: 18,
    humidity: 68,
    uvIndex: 5,
    pressureHpa: 1016,
    visibilityKm: 16,
    sunrise: '06:52',
    sunset: '19:10',
    hourly: [
      { time: 'Now', tempC: 19, icon: '⛅' },
      { time: '17:00', tempC: 19, icon: '⛅' },
      { time: '18:00', tempC: 18, icon: '⛅' },
      { time: '19:00', tempC: 17, icon: '🌅' },
      { time: '20:00', tempC: 15, icon: '🌙' },
      { time: '21:00', tempC: 14, icon: '🌙' },
      { time: '22:00', tempC: 14, icon: '☁️' },
      { time: '23:00', tempC: 13, icon: '☁️' },
    ],
    daily: [
      { day: 'Today', condition: 'Partly Cloudy', minC: 13, maxC: 22, icon: '⛅' },
      { day: 'Mon', condition: 'Sunny', minC: 14, maxC: 24, icon: '☀️' },
      { day: 'Tue', condition: 'Sunny', minC: 13, maxC: 23, icon: '☀️' },
      { day: 'Wed', condition: 'Cloudy', minC: 12, maxC: 18, icon: '☁️' },
      { day: 'Thu', condition: 'Rain', minC: 11, maxC: 16, icon: '🌧️' },
      { day: 'Fri', condition: 'Partly Cloudy', minC: 12, maxC: 19, icon: '⛅' },
      { day: 'Sat', condition: 'Sunny', minC: 14, maxC: 22, icon: '☀️' },
    ],
  },
  {
    city: 'London',
    country: 'United Kingdom',
    tempC: 15,
    condition: 'Rain',
    highC: 17,
    lowC: 10,
    windKmh: 24,
    humidity: 84,
    uvIndex: 2,
    pressureHpa: 1008,
    visibilityKm: 9,
    sunrise: '06:48',
    sunset: '18:55',
    hourly: [
      { time: 'Now', tempC: 15, icon: '🌧️' },
      { time: '17:00', tempC: 14, icon: '🌧️' },
      { time: '18:00', tempC: 14, icon: '🌦️' },
      { time: '19:00', tempC: 13, icon: '☁️' },
      { time: '20:00', tempC: 12, icon: '🌙' },
      { time: '21:00', tempC: 11, icon: '🌙' },
      { time: '22:00', tempC: 11, icon: '🌧️' },
      { time: '23:00', tempC: 10, icon: '🌧️' },
    ],
    daily: [
      { day: 'Today', condition: 'Rain', minC: 10, maxC: 17, icon: '🌧️' },
      { day: 'Mon', condition: 'Showers', minC: 9, maxC: 16, icon: '🌦️' },
      { day: 'Tue', condition: 'Cloudy', minC: 11, maxC: 18, icon: '☁️' },
      { day: 'Wed', condition: 'Sunny', minC: 10, maxC: 19, icon: '☀️' },
      { day: 'Thu', condition: 'Partly Cloudy', minC: 12, maxC: 20, icon: '⛅' },
      { day: 'Fri', condition: 'Rain', minC: 11, maxC: 15, icon: '🌧️' },
      { day: 'Sat', condition: 'Windy', minC: 9, maxC: 14, icon: '💨' },
    ],
  },
  {
    city: 'Tokyo',
    country: 'Japan',
    tempC: 26,
    condition: 'Sunny',
    highC: 28,
    lowC: 19,
    windKmh: 12,
    humidity: 55,
    uvIndex: 7,
    pressureHpa: 1018,
    visibilityKm: 20,
    sunrise: '05:32',
    sunset: '17:42',
    hourly: [
      { time: 'Now', tempC: 26, icon: '☀️' },
      { time: '17:00', tempC: 25, icon: '☀️' },
      { time: '18:00', tempC: 23, icon: '🌅' },
      { time: '19:00', tempC: 22, icon: '🌙' },
      { time: '20:00', tempC: 21, icon: '🌙' },
      { time: '21:00', tempC: 20, icon: '🌙' },
      { time: '22:00', tempC: 20, icon: '🌙' },
      { time: '23:00', tempC: 19, icon: '🌙' },
    ],
    daily: [
      { day: 'Today', condition: 'Sunny', minC: 19, maxC: 28, icon: '☀️' },
      { day: 'Mon', condition: 'Sunny', minC: 20, maxC: 29, icon: '☀️' },
      { day: 'Tue', condition: 'Partly Cloudy', minC: 21, maxC: 27, icon: '⛅' },
      { day: 'Wed', condition: 'Thunderstorm', minC: 18, maxC: 24, icon: '⛈️' },
      { day: 'Thu', condition: 'Rain', minC: 17, maxC: 22, icon: '🌧️' },
      { day: 'Fri', condition: 'Clear', minC: 18, maxC: 25, icon: '☀️' },
      { day: 'Sat', condition: 'Sunny', minC: 19, maxC: 27, icon: '☀️' },
    ],
  },
  {
    city: 'Johannesburg',
    country: 'South Africa',
    tempC: 23,
    condition: 'Sunny',
    highC: 26,
    lowC: 12,
    windKmh: 15,
    humidity: 40,
    uvIndex: 8,
    pressureHpa: 1022,
    visibilityKm: 25,
    sunrise: '06:05',
    sunset: '18:15',
    hourly: [
      { time: 'Now', tempC: 23, icon: '☀️' },
      { time: '17:00', tempC: 22, icon: '☀️' },
      { time: '18:00', tempC: 19, icon: '🌅' },
      { time: '19:00', tempC: 16, icon: '🌙' },
      { time: '20:00', tempC: 15, icon: '🌙' },
      { time: '21:00', tempC: 14, icon: '🌙' },
      { time: '22:00', tempC: 13, icon: '🌙' },
      { time: '23:00', tempC: 12, icon: '🌙' },
    ],
    daily: [
      { day: 'Today', condition: 'Sunny', minC: 12, maxC: 26, icon: '☀️' },
      { day: 'Mon', condition: 'Sunny', minC: 13, maxC: 27, icon: '☀️' },
      { day: 'Tue', condition: 'Thunderstorm', minC: 14, maxC: 25, icon: '⛈️' },
      { day: 'Wed', condition: 'Partly Cloudy', minC: 12, maxC: 23, icon: '⛅' },
      { day: 'Thu', condition: 'Sunny', minC: 11, maxC: 24, icon: '☀️' },
      { day: 'Fri', condition: 'Sunny', minC: 13, maxC: 26, icon: '☀️' },
      { day: 'Sat', condition: 'Clear', minC: 14, maxC: 28, icon: '☀️' },
    ],
  },
];

export const WeatherApp: React.FC = () => {
  const [selectedCity, setSelectedCity] = useState<CityWeather>(PRESET_CITIES[0]);
  const [useFahrenheit, setUseFahrenheit] = useState(false);
  const [search, setSearch] = useState('');

  const toDisplayTemp = (c: number) => {
    if (useFahrenheit) {
      return `${Math.round((c * 9) / 5 + 32)}°`;
    }
    return `${Math.round(c)}°`;
  };

  const getConditionIcon = (condition: string) => {
    switch (condition) {
      case 'Sunny':
        return <Sun className="w-14 h-14 text-amber-400 drop-shadow-lg" />;
      case 'Partly Cloudy':
        return <Cloud className="w-14 h-14 text-sky-300 drop-shadow-lg" />;
      case 'Cloudy':
        return <Cloud className="w-14 h-14 text-slate-300 drop-shadow-lg" />;
      case 'Rain':
        return <CloudRain className="w-14 h-14 text-blue-400 drop-shadow-lg" />;
      case 'Thunderstorm':
        return <CloudLightning className="w-14 h-14 text-purple-400 drop-shadow-lg" />;
      default:
        return <Sun className="w-14 h-14 text-amber-400" />;
    }
  };

  const getBackgroundTheme = () => {
    switch (selectedCity.condition) {
      case 'Sunny':
        return 'from-sky-500 via-blue-600 to-indigo-800';
      case 'Partly Cloudy':
        return 'from-blue-600 via-slate-700 to-slate-900';
      case 'Rain':
        return 'from-slate-700 via-slate-800 to-slate-950';
      case 'Thunderstorm':
        return 'from-slate-900 via-indigo-950 to-purple-950';
      default:
        return 'from-sky-600 via-blue-700 to-slate-900';
    }
  };

  const filteredCities = PRESET_CITIES.filter((c) =>
    c.city.toLowerCase().includes(search.toLowerCase())
  );

  return (
    <div
      className={`flex flex-col h-full w-full bg-gradient-to-b ${getBackgroundTheme()} text-white select-none overflow-hidden transition-all duration-700 font-sans`}
    >
      {/* Top Header & Search */}
      <div className="h-14 px-5 border-b border-white/10 bg-black/20 backdrop-blur-md flex items-center justify-between">
        <div className="flex items-center space-x-3">
          <div className="relative">
            <Search className="absolute left-2.5 top-1/2 -translate-y-1/2 w-3.5 h-3.5 text-white/50" />
            <input
              type="text"
              placeholder="Search City..."
              value={search}
              onChange={(e) => setSearch(e.target.value)}
              className="bg-white/15 border border-white/20 rounded-xl pl-8 pr-3 py-1 text-xs text-white placeholder-white/50 focus:outline-none focus:bg-white/25 w-44"
            />
          </div>
          {search && (
            <div className="flex space-x-1">
              {filteredCities.map((c) => (
                <button
                  key={c.city}
                  onClick={() => {
                    setSelectedCity(c);
                    setSearch('');
                  }}
                  className="px-2.5 py-1 rounded-lg bg-white/20 hover:bg-white/30 text-xs font-medium"
                >
                  {c.city}
                </button>
              ))}
            </div>
          )}
        </div>

        {/* Temperature Unit Toggle */}
        <div className="flex items-center space-x-1 bg-white/10 p-0.5 rounded-xl border border-white/15 text-xs font-semibold">
          <button
            onClick={() => setUseFahrenheit(false)}
            className={`px-2 py-0.5 rounded-lg transition-colors ${
              !useFahrenheit ? 'bg-white text-slate-900 shadow-sm' : 'text-white/70'
            }`}
          >
            °C
          </button>
          <button
            onClick={() => setUseFahrenheit(true)}
            className={`px-2 py-0.5 rounded-lg transition-colors ${
              useFahrenheit ? 'bg-white text-slate-900 shadow-sm' : 'text-white/70'
            }`}
          >
            °F
          </button>
        </div>
      </div>

      {/* Main Weather Overview */}
      <div className="flex-1 overflow-y-auto p-6 space-y-6">
        {/* City & Big Temperature */}
        <div className="flex flex-col items-center justify-center text-center space-y-1 py-4">
          <h1 className="text-3xl font-bold tracking-tight text-white drop-shadow-md">
            {selectedCity.city}
          </h1>
          <p className="text-xs font-medium text-white/80">{selectedCity.country}</p>
          <div className="text-7xl font-extralight tracking-tighter text-white py-2 drop-shadow-lg">
            {toDisplayTemp(selectedCity.tempC)}
          </div>
          <div className="flex items-center space-x-2 text-sm font-medium text-white/90">
            <span>{selectedCity.condition}</span>
            <span>•</span>
            <span>H: {toDisplayTemp(selectedCity.highC)}</span>
            <span>L: {toDisplayTemp(selectedCity.lowC)}</span>
          </div>
        </div>

        {/* Hourly Forecast Strip */}
        <div className="p-4 rounded-2xl bg-white/10 backdrop-blur-xl border border-white/15 space-y-3">
          <div className="text-[11px] font-semibold uppercase tracking-wider text-white/70">
            Hourly Forecast
          </div>
          <div className="flex items-center space-x-6 overflow-x-auto pb-1 scrollbar-none">
            {selectedCity.hourly.map((hour, idx) => (
              <div key={idx} className="flex flex-col items-center space-y-2 shrink-0">
                <span className="text-xs text-white/70">{hour.time}</span>
                <span className="text-xl">{hour.icon}</span>
                <span className="text-xs font-semibold">{toDisplayTemp(hour.tempC)}</span>
              </div>
            ))}
          </div>
        </div>

        {/* 7-Day Extended Forecast */}
        <div className="p-4 rounded-2xl bg-white/10 backdrop-blur-xl border border-white/15 space-y-2.5">
          <div className="text-[11px] font-semibold uppercase tracking-wider text-white/70">
            7-Day Forecast
          </div>
          <div className="space-y-2">
            {selectedCity.daily.map((day, idx) => (
              <div
                key={idx}
                className="flex items-center justify-between text-xs py-1 border-b border-white/5 last:border-0"
              >
                <span className="w-16 font-medium">{day.day}</span>
                <span className="text-lg w-8">{day.icon}</span>
                <span className="flex-1 text-white/70 text-right pr-4">{day.condition}</span>
                <div className="flex items-center space-x-2 w-28 justify-end font-mono">
                  <span className="text-white/60">{toDisplayTemp(day.minC)}</span>
                  <div className="w-14 h-1.5 rounded-full bg-white/20 overflow-hidden relative">
                    <div className="absolute inset-y-0 bg-amber-400 rounded-full w-2/3 left-1/6" />
                  </div>
                  <span className="font-semibold">{toDisplayTemp(day.maxC)}</span>
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* Atmospheric Details Grid */}
        <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
          <div className="p-3.5 rounded-2xl bg-white/10 backdrop-blur-xl border border-white/15 space-y-1">
            <div className="flex items-center space-x-1.5 text-white/70 text-[10px] uppercase font-semibold">
              <Wind className="w-3.5 h-3.5" />
              <span>Wind</span>
            </div>
            <div className="text-xl font-semibold font-mono">{selectedCity.windKmh} km/h</div>
            <p className="text-[10px] text-white/60">Gentle breeze from WNW</p>
          </div>

          <div className="p-3.5 rounded-2xl bg-white/10 backdrop-blur-xl border border-white/15 space-y-1">
            <div className="flex items-center space-x-1.5 text-white/70 text-[10px] uppercase font-semibold">
              <Droplets className="w-3.5 h-3.5" />
              <span>Humidity</span>
            </div>
            <div className="text-xl font-semibold font-mono">{selectedCity.humidity}%</div>
            <p className="text-[10px] text-white/60">Dew point is 12°</p>
          </div>

          <div className="p-3.5 rounded-2xl bg-white/10 backdrop-blur-xl border border-white/15 space-y-1">
            <div className="flex items-center space-x-1.5 text-white/70 text-[10px] uppercase font-semibold">
              <Sun className="w-3.5 h-3.5" />
              <span>UV Index</span>
            </div>
            <div className="text-xl font-semibold font-mono">{selectedCity.uvIndex}</div>
            <p className="text-[10px] text-white/60">Moderate UV exposure</p>
          </div>

          <div className="p-3.5 rounded-2xl bg-white/10 backdrop-blur-xl border border-white/15 space-y-1">
            <div className="flex items-center space-x-1.5 text-white/70 text-[10px] uppercase font-semibold">
              <Eye className="w-3.5 h-3.5" />
              <span>Visibility</span>
            </div>
            <div className="text-xl font-semibold font-mono">{selectedCity.visibilityKm} km</div>
            <p className="text-[10px] text-white/60">Clear horizon</p>
          </div>
        </div>
      </div>
    </div>
  );
};
