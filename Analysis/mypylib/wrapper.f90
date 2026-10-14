subroutine igrf_wrapper(colat, lon, r, date, Bx, By, Bz, f)
  implicit none

  double precision colat, lon, r, date
  double precision Bx, By, Bz, f

  double precision BN, BE, BD

  double precision theta, phi
  double precision st, ct, sp, cp

  integer itype

  !f2py intent(in) colat, lon, r, date
  !f2py intent(out) Bx, By, Bz, f

  itype = 2

  call igrf14syn(0, date, itype, r, colat, lon, BN, BE, BD, f)

  theta = colat * 3.141592653589793d0 / 180.d0
  phi   = lon   * 3.141592653589793d0 / 180.d0

  st = sin(theta)
  ct = cos(theta)
  sp = sin(phi)
  cp = cos(phi)

  Bx = -BD*st*cp - BN*ct*cp - BE*sp
  By = -BD*st*sp - BN*ct*sp + BE*cp
  Bz = -BD*ct + BN*st

end

subroutine rhs_particle(t, XV, qcharge, Ekg, date, dXV)
  implicit none

  double precision t, XV(6), dXV(6)
  double precision qcharge, Ekg, date

  double precision x,y,z,vx,vy,vz
  double precision r, theta, phi
  double precision colat, lon

  double precision BN, BE, BD
  double precision Bx, By, Bz
  double precision ax, ay, az

  double precision st, ct, sp, cp
  double precision factor

  integer itype

  !f2py intent(in) t, XV, qcharge, Ekg, date
  !f2py intent(out) dXV

  x  = XV(1)
  y  = XV(2)
  z  = XV(3)
  vx = XV(4)
  vy = XV(5)
  vz = XV(6)

  r = sqrt(x*x + y*y + z*z)
  theta = acos(z / r)
  phi = atan2(y, x)

  colat = theta * 180.d0 / 3.141592653589793d0
  lon   = phi   * 180.d0 / 3.141592653589793d0
  if (lon .lt. 0.d0) lon = lon + 360.d0

  itype = 2

  call igrf14syn(0, date, itype, r, colat, lon, BN, BE, BD, dXV(1))

  st = sin(theta)
  ct = cos(theta)
  sp = sin(phi)
  cp = cos(phi)

  Bx = -BD*st*cp - BN*ct*cp - BE*sp
  By = -BD*st*sp - BN*ct*sp + BE*cp
  Bz = -BD*ct + BN*st

  factor = qcharge / Ekg * 1d-9

  ax = (vy*Bz - vz*By) * factor
  ay = (vz*Bx - vx*Bz) * factor
  az = (vx*By - vy*Bx) * factor

  dXV(1) = vx
  dXV(2) = vy
  dXV(3) = vz
  dXV(4) = ax
  dXV(5) = ay
  dXV(6) = az

end

subroutine rhs_dparticle(t, XV, qcharge, Ekg, date, dXV)
  implicit none

  double precision t, XV(7), dXV(7)
  double precision qcharge, Ekg, date

  double precision x,y,z,vx,vy,vz,s
  double precision r, theta, phi
  double precision colat, lon

  double precision BN, BE, BD
  double precision Bx, By, Bz
  double precision ax, ay, az
  double precision sOt

  double precision st, ct, sp, cp
  double precision factor

  integer itype

  !f2py intent(in) t, XV, qcharge, Ekg, date
  !f2py intent(out) dXV

  x  = XV(1)
  y  = XV(2)
  z  = XV(3)
  vx = XV(4)
  vy = XV(5)
  vz = XV(6)
  s  = XV(7)

  r = sqrt(x*x + y*y + z*z)
  theta = acos(z / r)
  phi = atan2(y, x)

  colat = theta * 180.d0 / 3.141592653589793d0
  lon   = phi   * 180.d0 / 3.141592653589793d0
  if (lon .lt. 0.d0) lon = lon + 360.d0

  itype = 2

  call igrf14syn(0, date, itype, r, colat, lon, BN, BE, BD, dXV(1))

  st = sin(theta)
  ct = cos(theta)
  sp = sin(phi)
  cp = cos(phi)

  Bx = -BD*st*cp - BN*ct*cp - BE*sp
  By = -BD*st*sp - BN*ct*sp + BE*cp
  Bz = -BD*ct + BN*st

  factor = qcharge / Ekg * 1d-9

  ax = (vy*Bz - vz*By) * factor
  ay = (vz*Bx - vx*Bz) * factor
  az = (vx*By - vy*Bx) * factor
  sOt=sqrt(vx*vx+vy*vy+vz*vz)

  dXV(1) = vx
  dXV(2) = vy
  dXV(3) = vz
  dXV(4) = ax
  dXV(5) = ay
  dXV(6) = az
  dXV(7) = sOt

end