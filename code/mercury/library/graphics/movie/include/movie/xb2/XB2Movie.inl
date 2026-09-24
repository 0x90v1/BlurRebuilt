#ifndef MOVIE_XB2_MOVIE_INL
#define MOVIE_XB2_MOVIE_INL

namespace Movie
{

INLINE bool C_XB2Movie::IsCreated() const
{
	return m_pBink ? true : false;
}

}; // namespace Movie

#endif // MOVIE_XB2_MOVIE_INL
