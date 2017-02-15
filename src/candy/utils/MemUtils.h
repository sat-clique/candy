#ifndef X_14BE098C_C0B2_45B2_8097_2E20FE545C57_MEMUTILS_H
#define X_14BE098C_C0B2_45B2_8097_2E20FE545C57_MEMUTILS_H

namespace backported_std {
    /** Herb Sutter's make_unique implementation, see https://herbsutter.com/gotw/_102/
     * and http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique */
    template<typename T, typename ...Args>
    std::unique_ptr<T> make_unique( Args&& ...args )
    {
        return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
    }
}

#endif
