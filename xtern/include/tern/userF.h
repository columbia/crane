c----------------------------------------------------------
c----- Fortran Wrapper for xtern annotations
c----- ( iso_c_binding requires Fortran 2000+ compiler
c-----   e.g. gcc 4.3 + )
c----------------------------------------------------------
    
      interface
          subroutine soba_init( opaque_type, count,
     &               timeout_turns) bind ( c )
              use iso_c_binding
              integer(kind=c_int), VALUE :: opaque_type
              integer(kind=c_int), VALUE :: count
              integer(kind=c_int), VALUE :: timeout_turns
          end subroutine soba_init
          subroutine soba_wait( opaque_type) bind ( c )
              use iso_c_binding
              integer(kind=c_int), VALUE :: opaque_type
          end subroutine soba_wait
          subroutine pcs_enter() bind ( c )
              use iso_c_binding
          end subroutine pcs_enter
          subroutine pcs_exit() bind ( c )
              use iso_c_binding
          end subroutine pcs_exit
	  subroutine pcs_barrier_exit(bar_id, cnt) bind ( c )
              use iso_c_binding
              integer(kind=c_int), VALUE :: bar_id
              integer(kind=c_int), VALUE :: cnt
          end subroutine pcs_barrier_exit
      end interface
