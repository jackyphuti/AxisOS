import React from 'react';

interface AxisLogoProps {
  className?: string;
  size?: number;
  glow?: boolean;
  variant?: 'primary' | 'monochrome' | 'white' | 'black';
}

export const AxisLogo: React.FC<AxisLogoProps> = ({
  className = '',
  size = 24,
  glow = false,
  variant = 'primary',
}) => {
  const primaryGradientId = `axis-logo-grad-${Math.random().toString(36).substring(2, 9)}`;

  return (
    <div
      className={`relative inline-flex items-center justify-center select-none ${className}`}
      style={{ width: size, height: size }}
    >
      {glow && (
        <div
          className="absolute inset-0 rounded-full blur-md opacity-50 bg-[#87cf3e]"
          style={{ transform: 'scale(1.2)' }}
        />
      )}
      <svg
        width={size}
        height={size}
        viewBox="0 0 48 48"
        fill="none"
        xmlns="http://www.w3.org/2000/svg"
        className="relative z-10"
      >
        <defs>
          <linearGradient id={primaryGradientId} x1="6" y1="42" x2="42" y2="6" gradientUnits="userSpaceOnUse">
            <stop offset="0%" stopColor="#87cf3e" />
            <stop offset="60%" stopColor="#10b981" />
            <stop offset="100%" stopColor="#059669" />
          </linearGradient>
        </defs>

        {variant === 'primary' ? (
          <>
            {/* Outer Geometric Prism Ring */}
            <path
              d="M24 5L41 37H7L24 5Z"
              stroke={`url(#${primaryGradientId})`}
              strokeWidth="4"
              strokeLinejoin="round"
              strokeLinecap="round"
            />
            {/* Inner Axis Core Beam */}
            <path
              d="M24 16L32 32H16L24 16Z"
              fill={`url(#${primaryGradientId})`}
              fillOpacity="0.85"
            />
            {/* Horizontal Stabilizer Bar */}
            <circle cx="24" cy="23" r="3" fill="#FFFFFF" />
          </>
        ) : variant === 'white' ? (
          <>
            <path
              d="M24 5L41 37H7L24 5Z"
              stroke="#FFFFFF"
              strokeWidth="4"
              strokeLinejoin="round"
              strokeLinecap="round"
            />
            <path
              d="M24 16L32 32H16L24 16Z"
              fill="#FFFFFF"
              fillOpacity="0.85"
            />
            <circle cx="24" cy="23" r="3" fill="#0a0e0b" />
          </>
        ) : variant === 'black' ? (
          <>
            <path
              d="M24 5L41 37H7L24 5Z"
              stroke="#0a0e0b"
              strokeWidth="4"
              strokeLinejoin="round"
              strokeLinecap="round"
            />
            <path
              d="M24 16L32 32H16L24 16Z"
              fill="#0a0e0b"
              fillOpacity="0.85"
            />
            <circle cx="24" cy="23" r="3" fill="#87cf3e" />
          </>
        ) : (
          <>
            <path
              d="M24 5L41 37H7L24 5Z"
              stroke="currentColor"
              strokeWidth="4"
              strokeLinejoin="round"
              strokeLinecap="round"
            />
            <path
              d="M24 16L32 32H16L24 16Z"
              fill="currentColor"
              fillOpacity="0.85"
            />
            <circle cx="24" cy="23" r="3" fill="#FFFFFF" />
          </>
        )}
      </svg>
    </div>
  );
};
